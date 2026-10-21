//
// Created by Sande on 20.10.2026.
//

#pragma once
#include <atomic>
#include <coroutine>
#include <memory>
#include <vector>


class EventLoop {
public:
    struct IoState {
        std::coroutine_handle<> reader, writer;
        int fd;
    };

    /**
     * @brief Suspends the awaiting coroutine until the fd reports the event.
     */
    struct Awaiter {
        std::coroutine_handle<> IoState::*slot;
        IoState &io;

        static bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> h) {
            io.*slot = h;
        }
        static void await_resume() {}
    };

private:

    std::atomic<bool> running_{true};

    /**
     * @brief Epoll file descriptor
     */
    int epollFd_ = -1;

    /**
     * @brief eventFd
     */
    int wakeupFd_ = -1;

    /**
     * @brief closed connections;
     */
    std::vector<std::unique_ptr<IoState>> dead_;

public:

    EventLoop();
    ~EventLoop();

    EventLoop(EventLoop const&) = delete;
    EventLoop& operator=(EventLoop const&) = delete;
    EventLoop(EventLoop const&&) = delete;
    EventLoop&& operator=(EventLoop const&&) = delete;

    /**
     * @brief Registers fd . Loop thread only. nullptr on failure.
     */
    [[nodiscard]] std::unique_ptr<IoState> add(int fd) const;


    void run();

    void stop() noexcept;

    void remove(std::unique_ptr<IoState> io);


};
