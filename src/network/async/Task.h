#pragma once
#include <coroutine>
#include <exception>
#include <utility>

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
    void unhandled_exception() noexcept { exception = std::current_exception(); }
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

    
    template<typename P>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<P> handle) noexcept {
        auto continuation = handle.promise().continuation;
        return continuation ? continuation : std::noop_coroutine();
    }

    void await_resume() const noexcept {}
};


template<typename T = void>
class Task {
public:
    class promise_type : public PromiseBase {
    public:
        std::optional<T> value;

        Task get_return_object() { return Task(std::coroutine_handle<promise_type>::from_promise(*this)); }
        FinalAwaiter final_suspend() noexcept { return {}; }

        template<typename U>
        void return_value(U &&v) { value = std::forward<U>(v); }
    };

    explicit Task(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}
    Task(Task &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

    Task &operator=(Task &&other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = std::exchange(other.handle, nullptr);
        }
        return *this;
    }

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    ~Task() { if (handle) handle.destroy(); }

    // Lets one Task co_await another.
    [[nodiscard]] bool await_ready() const noexcept { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting) noexcept {
        handle.promise().continuation = awaiting;
        return handle;
    }

    T await_resume() {
        auto &promise = handle.promise();
        if (promise.exception) std::rethrow_exception(promise.exception);
        return std::move(*promise.value);
    }

private:
    std::coroutine_handle<promise_type> handle;
};

template<>
class Task<void> {
public:
    class promise_type : public PromiseBase {
    public:
        Task get_return_object() { return Task(std::coroutine_handle<promise_type>::from_promise(*this)); }
        static FinalAwaiter final_suspend() noexcept { return {}; }

        void return_void() noexcept {}
    };

    explicit Task(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}
    Task(Task &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

    Task &operator=(Task &&other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = std::exchange(other.handle, nullptr);
        }
        return *this;
    }

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    ~Task() { if (handle) handle.destroy(); }

     [[nodiscard]] static bool await_ready() noexcept { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting) noexcept {
        handle.promise().continuation = awaiting;
        return handle;
    }

    void await_resume() {
        if (handle.promise().exception) std::rethrow_exception(handle.promise().exception);
    }

private:
    std::coroutine_handle<promise_type> handle;
};


class DetachedTask {
public:
    struct promise_type {
        static std::suspend_never initial_suspend() noexcept { return {}; }
        static std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}


        static void unhandled_exception() {
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const std::exception &e) {
                Logger::log("Unhandled exception in connection coroutine" + std::string(e.what()) + ":" , errno);
            } catch (...) {
                Logger::log("Unhandled unknown exception in connection coroutine! Error: ", -1);
            }

        }

        DetachedTask get_return_object() noexcept { return {}; }
    };
};