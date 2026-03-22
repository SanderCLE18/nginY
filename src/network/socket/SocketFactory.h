//
// Created by Sande on 21.03.2026.
//
#pragma once
#include <string>

class SocketFactory {
public:
    virtual int createListenSocket(const std::string& port) = 0;
    virtual int createClientSocket(int socket) = 0;
    virtual ~SocketFactory() = default;

};