//
// Created by Sande on 01.03.2026.
//
#pragma once
#include <optional>
#include <sys/ioctl.h>
#include <sys/types.h>

class Connection {

public:
    virtual ssize_t read(void* buf, size_t len) = 0;
    virtual ssize_t write(const void* buf, size_t len) = 0;
    virtual void close() = 0;
    virtual void shutdown(std::optional<int> how) = 0;

    virtual ~Connection() = default;

    void setBlocking(bool blocking) {
        unsigned long mode = blocking ? 0 : 1;
        ioctl(fd, FIONBIO, &mode);
    }

protected:
    int fd;
    
};


