//
// Created by Sande on 01.03.2026.
//
#pragma once
#include <optional>
#include <sys/ioctl.h>
#include <sys/types.h>

/**
 * @class Connection
 * @brief An interface outlining the different functions for the different connections.
 */
class Connection {

public:

    /**
     * @brief Reads data from a connection
     *
     * @param buf Pointer to the buffer where data is stored
     * @param len Maximum number of bytes to read
     * @return bytes read or -1 on error
     */
    virtual ssize_t read(void* buf, size_t len) = 0;

    /**
     * @brief Writes data to a connection
     *
     * @param buf Pointer to the buffer where data is written
     * @param len Number of bytes to write
     * @return Number of bytes written or -1 on error
     */
    virtual ssize_t write(const void* buf, size_t len) = 0;

    /**
     * @brief Closes the connection
     */
    virtual void close() = 0;

    /**
     * @brief Shutdown part of a connection or fully.
     *
     * @param how Optional integer describing how the connection is shut down
     */
    virtual void shutdown(std::optional<int> how) = 0;

    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes
     */
    virtual ~Connection() = default;
    
};


