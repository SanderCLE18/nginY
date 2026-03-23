//
// Created by Sande on 21.03.2026.
//
#pragma once
#include <string>

/**
 * @brief SocketFactory is an abstract class that defines the interface for creating sockets.
 */
class SocketFactory {
public:
    /**
     * @brief Virtual method for creating a listening socket.
     *
     * @param port The port number to listen on.
     * @return The file descriptor of the created socket.
     */
    virtual int createListenSocket(const std::string& port) = 0;

    /**
     * @brief Virtual method for creating a listening socket.
     *
     * @param host The host address to listen on. Nullptr / all if empty
     * @param port The port number to listen on.
     * @return The file descriptor of the created socket.
     */
    virtual int createListenSocket(const std::string& host, const std::string& port) = 0;

    /**
     * @brief Virtual method for creating a client socket.
     *
     * @param socket socket
     * @return file descriptor
     */
    virtual int createClientSocket(int socket) = 0;

    /**
     * @brief Destructor for SocketFactory.
     */
    virtual ~SocketFactory() = default;

};