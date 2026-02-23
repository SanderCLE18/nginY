#pragma once

#include <mutex>
#include <future>


class ThreadPool {
private:


public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    template <typename F, typename... Args>
    auto submit(std::function<void()> task) -> std::future<std::invoke_result_t<F, Args...>>;
};