//
// Created by Sande on 21.03.2026.
//

#include <gtest/gtest.h>
#include <string>
#include <../tests/network/socket/gTestSocketFactory.h>
#include "core/WebServer.h"
#include <../tests/network/connections/gTestConnection.cpp>

using ::testing::Return;
using ::testing::_;

class WebServerTest : public ::testing::Test {
friend class WebServer;

protected:
    std::unique_ptr<WebServer> server;
    gTestSocketFactory factory;
public:
    void SetUp() override {
        EXPECT_CALL(factory, createListenSocket(_))
            .WillRepeatedly(testing::Return(3)); // fake fd
        server = std::make_unique<WebServer>("", factory);
    }
};

TEST_F(WebServerTest, constructorCallsListenSockets) {
    EXPECT_CALL(factory, createListenSocket("8080")).Times(1);


    server = std::make_unique<WebServer>("", factory);
}

TEST_F(WebServerTest, constructorCanHandleError) {
    EXPECT_CALL(factory, createListenSocket(_))
        .WillOnce(testing::Throw(std::runtime_error("bind failed")));

    EXPECT_NO_THROW(std::make_unique<WebServer>("", factory));
}

TEST_F(WebServerTest, serveStaticCreatesHeader) {
    gTestConnection connection;

    std::string url = "/index.html";

    EXPECT_CALL(connection, write(_,_)).Times(testing::AtLeast(1));
    server->serveStatic(url, connection);
}

TEST_F(WebServerTest, serveStaticNotFoundOnlyHeader) {
    gTestConnection connection;

    std::string url = "/index2.html";

    EXPECT_CALL(connection, write(_,_)).Times(1);
    server->serveStatic(url, connection);
}

TEST_F(WebServerTest, serveStaticSimpleSlash) {
    gTestConnection connection;

    std::string url = "/";

    EXPECT_CALL(connection, write(_,_)).Times(testing::AtLeast(1));
    server->serveStatic(url, connection);
}

TEST_F(WebServerTest, createClientRouteToStatic) {
    auto connection = std::make_unique<gTestConnection>();
    std::string request = "GET /index.html HTTP/1.1\r\n\r\n";

    EXPECT_CALL(*connection, read(_,_))
    .WillOnce([&](void* buf, size_t size) {
        memcpy(buf, request.data(), request.size());
        return static_cast<ssize_t>(request.size());
    });
    EXPECT_CALL(*connection, write(_,_)).Times(testing::AtLeast(1));
    EXPECT_CALL(*connection, shutdown(_)).Times(1);
    EXPECT_CALL(*connection, close()).Times(1);

    server->createClientThread(std::move(connection));
}

TEST_F(WebServerTest, createClientThreadFailure) {
    std::unique_ptr<gTestConnection> connection = std::make_unique<gTestConnection>();

    EXPECT_CALL(*connection, read(_, _)).WillOnce(testing::Return(-1));
    EXPECT_CALL(*connection, shutdown(_)).Times(1);
    EXPECT_CALL(*connection, close()).Times(1);

    EXPECT_NO_THROW(server->createClientThread(std::move(connection)));
}