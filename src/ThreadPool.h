#pragma once

#include <mutex>
#include <future>
#include <functional>
#include <thread>
#include <condition_variable>
#include <vector>


class ThreadPool {
private:
    std::vector<std::thread> threads;
    std::mutex mutex;

    std::vector<std::function<void()>> tasks;
    std::condition_variable condition;

    bool running;

public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    template <class F, class... Args>
    auto submit(F&& f, Args&& args) -> std::future<typename std::result_of<F(Args...)>::type>;
};

inline ThreadPool::ThreadPool(size_t num_threads) : running(false)
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
                    this->condition.wait(lock, [this]() { return this->running || !this->tasks.empty(); });
                    if (this->running && this->tasks.empty()) return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop_front();
                }
                task();
            }
        }));
    }
}
inline ThreadPool::~ThreadPool()
{
    {
        mutex.lock();
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
{
    using return_type = typename std::result_of<F(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(this->mutex);
        if (this->running)
        {
            throw std::runtime_error("ThreadPool already running");
        }
        this->tasks.emplace_back([task]() { (*task)(); });

        condition.notify_one();
        return res;

    }
}
