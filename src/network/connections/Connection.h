//
// Created by Sande on 01.03.2026.
//
#pragma once
#include <sys/types.h>

class Connection {

public:
    virtual ssize_t read(void* buf, size_t len) = 0;
    virtual ssize_t write(const void* buf, size_t len) = 0;
    virtual void close() = 0;

    virtual ~Connection() = default;

protected:
    int fd;
    
};


