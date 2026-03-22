//
// Created by Sande on 21.03.2026.
//
#pragma once
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <network/socket/SocketFactory.h>


class GTestSocketFactory : public SocketFactory {

public:
    MOCK_METHOD(int, createListenSocket, (const std::string& port), (override));
    MOCK_METHOD(int, createClientSocket, (int socket), (override));

};

