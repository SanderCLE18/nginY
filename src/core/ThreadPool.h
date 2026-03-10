#pragma once

#include <mutex>
#include <future>
#include <functional>
#include <thread>
#include <condition_variable>
#include <vector>
#include <queue>

class ThreadPool {
private:
    std::vector<std::thread> threads;
    std::mutex mutex;

    std::queue<std::function<void()>> tasks;
    std::condition_variable condition;

    bool running;

public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    template <class F, class... Args>
    auto submit(F&& f, Args&& ...args) -> std::future<typename std::invoke_result_t<F, Args...>>;
};

inline ThreadPool::ThreadPool(size_t num_threads) : running(true)
{
    for (size_t i = 0; i < num_threads; i++)
    {
        threads.emplace_back(std::thread([this]
        {
            for (;;)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->mutex);
                    this->condition.wait(lock, [this]() { return !this->running || !this->tasks.empty(); });
                    if (!this->running && this->tasks.empty()) return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        }));
    }
}
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        running = false;
    }
    condition.notify_all();
    for (auto& thread : threads)
    {
        thread.join();
    }
}

template <class F, class... Args>
auto ThreadPool::submit(F&& f, Args&& ...args)
    -> std::future<typename std::invoke_result_t<F, Args...>>
{
    using return_type = typename std::invoke_result_t<F, Args...>;
    auto boundArgs = std::tuple(std::forward<Args>(args)...);

    auto task = std::make_shared<std::packaged_task<return_type()> >(
        [f = std::forward<F>(f), args = std::move(boundArgs)]() mutable {
            return std::apply(f, std::move(args));
    });

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(this->mutex);
        if (!this->running)
        {
            throw std::runtime_error("ThreadPool not running");
        }
        this->tasks.push([task]() { (*task)(); });

        condition.notify_one();
        return res;

    }
}
