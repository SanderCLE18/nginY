//
// Created by Sande on 20.10.2026.
//

#include "../../core/EventLoop.hpp"
#include <utility>
#include <sys/epoll.h>
#include "../../utils/Logger.h"

#include "../../core/IoHandle.h"

std::unique_ptr<EventLoop::IoState> EventLoop::add(const int fd) const {
    auto io = std::make_unique<IoState>(IoState{.fd = fd});
    epoll_event event{};
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLOUT;
    event.data.ptr = &io;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == -1) {
        Logger::log("Error adding socket to epoll: ", errno);
        return nullptr;
    }
    return io;
}

void EventLoop::remove(std::unique_ptr<IoState> io) {
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, io->fd, nullptr);
    ::close(io->fd);
    dead_.push_back(std::move(io));

}

void EventLoop::run() {
    constexpr int FD_MAX = 2;
    constexpr int TIMEOUT = 10;

    auto epollEvents = std::make_unique<struct epoll_event[]>(FD_MAX);


    while (running_) {
        const int readyFd = epoll_wait(epollFd_, epollEvents.get(), FD_MAX, TIMEOUT);
        if (readyFd < 0) {
            Logger::log("Error in epoll_wait: ", errno);
        }
        for (int i = 0; i < readyFd; ++i) {
            auto *tag = static_cast<Tag *>(epollEvents[i].data.ptr);
        }
    }
}

EventLoop::EventLoop() {
    epollFd_ = epoll_create1(EPOLL_CLOEXEC);
}

EventLoop::~EventLoop() {
    running_.store(false, std::memory_order_relaxed);
    if (epollFd_) {
        close(epollFd_);
    }
}
