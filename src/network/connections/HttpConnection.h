//
// Created by Sande on 06.03.2026.
//

#pragma once
#include "Connection.h"
#include <string>
#include <unistd.h>
#include <bits/socket.h>
#include <sys/socket.h>

class HttpConnection : Connection {

public:
    HttpConnection(int socket) {
        this->fd = socket;
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
};
