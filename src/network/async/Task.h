#pragma once
#include <coroutine>
#include <exception>
#include <utility>
#include <optional>
#include <type_traits>

#include "../../utils/Logger.h"

/**
 * Shared base for both promise types so the continuation/exception isn't duplicated
 */
class PromiseBase {
public:

    std::coroutine_handle<> continuation = nullptr;
    std::exception_ptr exception = nullptr;

    /**
     * Method for  making the Task "lazy".
     * The coroutine body is created but doesn't run until something explicitly resumes it.
     *
     * @return returns awaiter
     */
    static std::suspend_always initial_suspend() noexcept { return {}; }

    /**
     * Called by compiler when an exception escapes the coroutine body. Captures and rethrows later.
     */
    void unhandled_exception() noexcept {
        exception = std::current_exception();
    }
};


/**
 * The awaiter returned from final_suspend(). Symmetric transfer mechanism
 */
struct FinalAwaiter {
    /**
     * Tells the program not to finish the coroutine until the result/exception has been read.
     *
     * @return false
     */
    static bool await_ready() noexcept { return false; }

    /**
     * Passes on the control directly to the next coroutine 
     *
     * @tparam P Promise type
     * @param handle the handle of the finishing coroutine.
     * @return returns the continuation's coroutine handle.
     */
    template<typename P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> handle) noexcept {
        auto continuation = handle.promise().continuation;
        return continuation ? continuation : std::noop_coroutine();
    }

    /**
     * Does nothing, resumed into as result of final_suspend
     */
    void await_resume() const noexcept {}
};

template<typename T>
class Promise;

/**
 * Owns the coroutine handle for every Task: move semantics, destruction and being co_awaited.
 * Only what depends on the result type is left to the derived Task.
 *
 * @tparam P the promise type of the coroutine
 */
template<typename P>
class TaskBase {
public:

    /**
     * Move constructor, transfers ownership and nulls out the source.
     *
     * @param other the item to be owned.
     */
    TaskBase(TaskBase &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

    /**
     * Copy constructor. Deletes the copy as a coroutine frame can not have dual ownership.
     *
     * @param other the task
     * @return returns a pointer to the item
     */
    TaskBase &operator=(TaskBase &&other) noexcept {
        if (this != &other) {
            if (handle) {
                handle.destroy();
            }
            handle = std::exchange(other.handle, nullptr);
        }
        return *this;
    }

    TaskBase(const TaskBase &) = delete;

    TaskBase &operator=(const TaskBase &) = delete;

    ~TaskBase() {
        if (handle) {
            handle.destroy();
        }
    }

    [[nodiscard]] static bool await_ready() noexcept {
        return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting) noexcept {
        handle.promise().continuation = awaiting;
        return handle;
    }

protected:
    explicit TaskBase(std::coroutine_handle<P> h) noexcept : handle(h) {}

    std::coroutine_handle<P> handle;
};


/**
 * Lazy coroutine return type. Covers T = void.
 *
 * @tparam T the type produced by co_return
 */
template<typename T = void>
class Task : public TaskBase<Promise<T>> {
public:
    using promise_type = Promise<T>;

    explicit Task(std::coroutine_handle<promise_type> h) noexcept : TaskBase<promise_type>(h) {}

    T await_resume() {
        auto &promise = this->handle.promise();
        if (promise.exception) {
            std::rethrow_exception(promise.exception);
        }
        if constexpr (!std::is_void_v<T>) {
            return std::move(*promise.value);
        }
    }
};

template <typename T>
class Promise : public PromiseBase {
public:
    std::optional<T> value;

    Task<T> get_return_object() noexcept {
        return Task<T>(std::coroutine_handle<Promise>::from_promise(*this) );
    }

    FinalAwaiter final_suspend() noexcept {
        return {};
    }
    template<typename U>
    void return_value(U &&v) {
        value = std::forward<U>(v);
    }
};

template<>
class Promise<void> : public PromiseBase {
public:

    Task<void> get_return_object() noexcept {
        return Task<void>(std::coroutine_handle<Promise>::from_promise(*this) );
    }

    FinalAwaiter final_suspend() noexcept {
        return {};
    }

    void return_void() noexcept {}
};

/**
 * Fire and forget for per-connection coroutines.
 */
class DetachedTask {
public:
    struct promise_type {
        /**
         * Makes coroutine run immediately when called.
         *
         * @return nothing.
         */
        static std::suspend_never initial_suspend() noexcept { return {}; }

        /**
         * Called when a coroutine finishes, self-cleaning.
         *
         * @return nothing.
         */
        static std::suspend_never final_suspend() noexcept { return {}; }

        /**
         * No operation
         */
        void return_void() noexcept {}

        /**
         *  Logs and closes the connection (for later)
         */
        static void unhandled_exception() {
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const std::exception &e) {
                Logger::log("Unhandled exception in connection coroutine" + std::string(e.what()) + ":" , errno);
            } catch (...) {
                Logger::log("Unhandled unknown exception in connection coroutine! Error: ", -1);
            }

        }

        /**
         * Returns an empty DetachedTasked as there is no handle to hold onto
         *
         * @return returns an empty task
         */
        static DetachedTask get_return_object() noexcept { return {}; }
    };
};