//
// Created by Sande on 20.10.2026.
//

#pragma once
#include "EventLoop.hpp"

/**
 * Wrapper to make the IoState move-only for connections.
 */
class IoHandle {
    EventLoop* loop;
    std::unique_ptr<EventLoop::IoState> state_;

    public:
    IoHandle(EventLoop& loop, int fd) : loop(&loop), state_(loop.add(fd)) {}
    ~IoHandle() {
        if (state_) {
            loop->remove(std::move(state_));
        }
    }
    [[nodiscard]] EventLoop::IoState &state() const {
        return *state_;
    }

};
