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
    /**
     * @brief Constructor for HttpConnection
     *
     * @param socket Incoming connection
     */
    HttpConnection(int socket) {
        this->fd = socket;
        setBlocking(true);
    }

    /**
     * @brief Reads data from an HTTP connection
     *
     * @param buffer Pointer to the buffer where data is stored
     * @param size Size of the buffer
     * @return Number of bytes actually read
     */
    ssize_t read(void* buffer, size_t size) override {
        return recv(this->fd, buffer, size, 0);
    }

    /**
     * @brief Writes data to a connection
     *
     * @param buffer Pointer to the buffer where data is to be written
     * @param len Length of the data to be written
     * @return Number of bytes actually written
     */
    ssize_t write(const void* buffer, size_t len) override {
        return send(this->fd, buffer, len, MSG_NOSIGNAL);
    }

    /**
     * @brief Closes the connection
     */
    void close() override {
        ::close(this->fd);
    }

    /**
     * @brief Shuts down the connection, either fully or partially.
     *
     * @param how Describes how the
     */
    void shutdown(std::optional<int> how) override {
        if (!how.has_value()) {
            ::shutdown(this->fd, 0);
        }
        else {
            ::shutdown(this->fd, how.value());
        }

    }

    /**
    * @brief Sets the connection to either blocking or non-blocking mode
    *
    * @param blocking If true, connections will block
    */
    void setBlocking(bool blocking) const {
        unsigned long mode = blocking ? 0 : 1;
        ioctl(fd, FIONBIO, &mode);
    }

protected:
    /**
     * @brief File descriptor for the connection
     */
    int fd;
};
