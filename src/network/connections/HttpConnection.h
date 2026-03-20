//
// Created by Sande on 06.03.2026.
//

#pragma once
#include "Connection.h"
#include <string>
#include <unistd.h>
#include <bits/socket.h>
#include <sys/socket.h>

class HttpConnection : public Connection {

public:
    HttpConnection(int socket) {
        this->fd = socket;
        setBlocking(true);
    }

    ssize_t read(void* buffer, size_t size) override {
        return recv(this->fd, buffer, size, 0);
    }
    ssize_t write(const void* buffer, size_t len) override {
        return send(this->fd, buffer, len, MSG_NOSIGNAL);
    }
    void close() override {
        ::close(this->fd);
    };
    void shutdown(std::optional<int> how) override {
        if (!how.has_value()) {
            ::shutdown(this->fd, -1);
        }
        else {
            ::shutdown(this->fd, how.value());
        }

    }
};
