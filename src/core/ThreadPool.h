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

    std::queue<std::function<void()> > tasks;
    std::condition_variable condition;

    bool running;

public:
    /**
    * Creates a ThreadPool with the specified number of threads.
    *
    * @param num_threads number of threads to create in the thread pool
    */
    ThreadPool(size_t num_threads);

    /**
    * Destructor for ThreadPool.
    * Shuts down the thread pool and waits for all threads to finish before destruction.
    */
    ~ThreadPool();

    /**
    * Submits a function to the thread pool for execution.
    *
    * @tparam F template function type
    * @tparam Args template arguments
    * @param f function to submit
    * @param args function arguments
    * @return the result of the submitted function
    *
    * @throw std::runtime_error if the thread pool is not running
    */
    template<class F, class... Args>
    auto submit(F &&f, Args &&... args) -> std::future<typename std::invoke_result_t<F, Args...> >;

    /**
    * Shuts down the thread pool, joining all threads.
    */
    void shutdown();
};

inline ThreadPool::ThreadPool(size_t num_threads) : running(true) {
    for (size_t i = 0; i < num_threads; i++) {
        threads.emplace_back(std::thread([this] {
            for (;;) {
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

inline ThreadPool::~ThreadPool() {
    shutdown();
}


template<class F, class... Args>
auto ThreadPool::submit(F &&f, Args &&... args)
    -> std::future<typename std::invoke_result_t<F, Args...> > {
    using return_type = typename std::invoke_result_t<F, Args...>;
    auto boundArgs = std::tuple(std::forward<Args>(args)...);

    auto task = std::make_shared<std::packaged_task<return_type()> >(
        [f = std::forward<F>(f), args = std::move(boundArgs)]() mutable {
            return std::apply(f, std::move(args));
        });

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(this->mutex);
        if (!this->running) {
            throw std::runtime_error("ThreadPool not running");
        }
        this->tasks.push([task]() { (*task)(); });

        condition.notify_one();
        return res;
    }
}


inline void ThreadPool::shutdown() {
    {
        std::unique_lock lock(mutex);
        if (!running) return;
        running = false;
    }
    condition.notify_all();
    for (auto &thread: threads) {
        thread.join();
    }
}
