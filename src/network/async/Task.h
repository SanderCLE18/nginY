#pragma once
#include <coroutine>
#include <exception>
#include <utility>

namespace detail {
    // Shared continuation/exception state for both the value and void promises.
    class PromiseBase {
    public:
        std::coroutine_handle<> continuation = nullptr;
        std::exception_ptr exception = nullptr;

        std::suspend_always initial_suspend() noexcept { return {}; }

        void unhandled_exception() noexcept { exception = std::current_exception(); }
    };

    // final_suspend's awaiter: symmetric transfer into whoever is awaiting us,
    // instead of resuming them from inside our own stack frame.
    struct FinalAwaiter {
        bool await_ready() const noexcept { return false; }

        template<typename Promise>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> handle) noexcept {
            auto continuation = handle.promise().continuation;
            return continuation ? continuation : std::noop_coroutine();
        }

        void await_resume() const noexcept {}
    };
}

template<typename T = void>
class Task {
public:
    class promise_type : public detail::PromiseBase {
    public:
        std::optional<T> value;

        Task get_return_object() { return Task(std::coroutine_handle<promise_type>::from_promise(*this)); }
        detail::FinalAwaiter final_suspend() noexcept { return {}; }

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

// void specialization -- same shape, no stored value.
template<>
class Task<void> {
public:
    class promise_type : public detail::PromiseBase {
    public:
        Task get_return_object() { return Task(std::coroutine_handle<promise_type>::from_promise(*this)); }
        detail::FinalAwaiter final_suspend() noexcept { return {}; }

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

    [[nodiscard]] bool await_ready() const noexcept { return false; }

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

// Fire-and-forget variant for per-connection coroutines: starts eagerly, no
// handle kept by the caller, cleans itself up on its own final_suspend.
class DetachedTask {
public:
    struct promise_type {
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}

        // TODO: decide real policy -- terminate, or log + close the connection?
        static void unhandled_exception() { std::terminate(); }

        DetachedTask get_return_object() noexcept { return {}; }
    };
};