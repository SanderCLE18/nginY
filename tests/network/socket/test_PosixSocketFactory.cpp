//
// Created by Sande on 22.03.2026.
//

#include <gtest/gtest.h>

#include "../../../src/network/socket/PosixSocketFactory.h"



TEST(PosixSocketFactoryTest, canCreateSocket) {
    PosixSocketFactory factory;
    int socket = factory.createListenSocket("0");

    EXPECT_GT(socket, 0);
    close(socket);
}

TEST(PosixSocketFactoryTest, throwsOnInvalidPort) {
    PosixSocketFactory factory;

    EXPECT_THROW({
            int socket = factory.createListenSocket("invalid-port-innit");
        }, std::runtime_error);

}