//
// Created by Sande on 21.03.2026.
//

#include <gtest/gtest.h>
#include "gTestConnection.cpp"
#include "gTestConnectionParser.h"

using ::testing::Return;
using ::testing::_;

TEST(connectionTests, WritesDataCorrectly) {
    gTestConnection testConnection;
    const std::string data = "test";

    EXPECT_CALL(testConnection, write(testing::_, 4))
        .WillOnce(testing::Return(4));

    ssize_t written = testConnection.write(const_cast<char*>(data.c_str()), data.size());
    EXPECT_EQ(written, 4);

}

TEST(connectionTests, canRead) {
    gTestConnection connection;
    gTestConnectionParser parser(connection);

    testing::InSequence seq;
    for (char c : std::string("POST")) {
        EXPECT_CALL(connection, read(testing::_, 1))
            .WillOnce([c](void* buf, size_t) {
                memcpy(buf, &c, 1);
                return static_cast<ssize_t>(1);
            });
    }
    EXPECT_CALL(connection, read(testing::_, 1))
    .WillOnce([](void* buf,  size_t) {
        memcpy(buf, "\n", 1);
        return static_cast<ssize_t>(1);
    });

    EXPECT_EQ(parser.readLine(), "POST");
}

TEST(connectionTests, canReadClosed) {
    gTestConnection testConnection;
    gTestConnectionParser parser(testConnection);

    EXPECT_CALL(testConnection, read(testing::_, 1))
    .WillOnce(testing::Return(0));

    EXPECT_EQ(parser.readLine(), "");
}

TEST(connectionTests, canReadError) {
    gTestConnection testConnection;
    gTestConnectionParser parser(testConnection);

    EXPECT_CALL(testConnection, read(testing::_, 1))
    .WillOnce(testing::Return(-1));

    EXPECT_EQ(parser.readLine(), "");
}

TEST(connectionTests, canShutdown) {
    gTestConnection testConnection; 
    
    EXPECT_CALL(testConnection, shutdown(std::optional<int>(0))).Times(1);
    
    testConnection.shutdown(0);
}
