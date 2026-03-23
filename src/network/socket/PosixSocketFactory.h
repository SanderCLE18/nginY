//
// Created by Sande on 21.03.2026.
//

#pragma once
#include <cstring>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include "SocketFactory.h"

class PosixSocketFactory : public SocketFactory {
public:
    int createListenSocket(const std::string& port) override;
    int createClientSocket(int socket) override;
};

inline int PosixSocketFactory::createListenSocket(const std::string& port) {
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int result = getaddrinfo(nullptr, port.c_str(), &hints, &res);
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
    ioctl(ListenSocket, FIONBIO, &mode);

    return ListenSocket;
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
