//
// Created by Sande on 21.03.2026.
//

#pragma once
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include "SocketFactory.h"

/**
 * @brief Factory for creating POSIX sockets.
 * This class provides a concrete implementation of the SocketFactory
 * interface using the POSIX socket API
 *
 */
class PosixSocketFactory : public SocketFactory {
public:
    /**
     * @brief Creates a listening socket for the given port.
     *
     * @param port reference to the port string
     * @return file descriptor
     */
    int createListenSocket(const std::string& port) override;

    /**
     * @brief Creates a listening socket for the given port.
     *
     * @param host reference to the host string, host for getaddrinfo
     * @param port reference to the port string
     * @return file descriptor
     */
    int createListenSocket(const std::string& host, const std::string& port);

    /**
     * @brief Creates a client socket for the given socket.
     *
     * @param socket reference to the socket
     * @return file descriptor
     */
    int createClientSocket(int socket) override;
};

inline int PosixSocketFactory::createListenSocket(const std::string& host, const std::string& port) {
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    const char* host_ptr = host.empty() ? nullptr : host.c_str();
    int result = getaddrinfo(host_ptr, port.c_str(), &hints, &res);

    if (result != 0) {
        throw std::runtime_error("getaddrinfo failed: " + std::to_string(result));
    }
    //set socket

    int ListenSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (ListenSocket == -1) {
        freeaddrinfo(res);
        throw std::runtime_error("Error: Failed to create listening socket: " + std::to_string(errno));
    }
    //bind
    int opt = 1;
    setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int listenResult = bind(ListenSocket, res->ai_addr, static_cast<int>(res->ai_addrlen));
    if (listenResult == -1)	{
        freeaddrinfo(res);
        throw std::runtime_error("Error: Failed to bind listening socket: " + std::to_string(errno));
    }
    freeaddrinfo(res);
    //listen
    if (listen(ListenSocket, SOMAXCONN) == -1) {
        throw std::runtime_error("Error: Failed to listen on socket: " + std::to_string(errno));
    }
    unsigned long mode = 1;
    fcntl(ListenSocket, F_SETFL, O_NONBLOCK);

    return ListenSocket;
}
inline int PosixSocketFactory::createListenSocket(const std::string& port) {
    return createListenSocket("", port);
}
inline int PosixSocketFactory::createClientSocket(int socket) {
    //make client socket
    int Client;
    Client = accept(socket, nullptr, nullptr);
    if (Client == -1 && errno != EWOULDBLOCK) {
        throw std::runtime_error("Error: Failed to accept incoming connection: " + std::to_string(errno));
    }
    return Client;
}
