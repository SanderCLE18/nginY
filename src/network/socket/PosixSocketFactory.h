//
// Created by Sande on 21.03.2026.
//

#pragma once
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

    /**
    * @brief Method for connecting an incoming connection to the backend.
    *
    * @param host The target host.
    * @param port The port to redirect to.
    * @return returns the filedescriptor of the new listen socket.
    */
    int connectSocket(const std::string& host, const std::string& port) override;
};

inline int PosixSocketFactory::createListenSocket(const std::string& host, const std::string& port) {
    addrinfo hints{}, *res;

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
    int theSocket = -1;
    for (addrinfo* a = res; a != nullptr; a = a->ai_next) {
        theSocket = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (theSocket == -1) {
            continue;
        }
        int opt = 1;
        setsockopt(theSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(theSocket, res->ai_addr, static_cast<int>(res->ai_addrlen)) == 0) {
            break;
        }
        close(theSocket);
        theSocket = -1;
    }
    freeaddrinfo(res);
    //listen
    if (listen(theSocket, SOMAXCONN) == -1) {
        throw std::runtime_error("Error: Failed to listen on socket: " + std::to_string(errno));
    }
    fcntl(theSocket, F_SETFL, O_NONBLOCK);

    return theSocket;
}

inline int PosixSocketFactory::connectSocket(const std::string& host, const std::string& port) {
    addrinfo hints{}, *res;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    const char* host_ptr = host.empty() ? nullptr : host.c_str();
    int result = getaddrinfo(host_ptr, port.c_str(), &hints, &res);

    if (result != 0) {
        throw std::runtime_error("getaddrinfo failed: " + std::to_string(result));
    }
    //set socket
    int fd = -1;
    for (addrinfo* a = res; a != nullptr; a = a->ai_next) {
        fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (fd == -1) {
            continue;
        }
        if (connect(fd, a->ai_addr, static_cast<int>(a->ai_addrlen)) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    return fd;
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
