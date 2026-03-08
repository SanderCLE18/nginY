//
// Created by Sande on 09.02.2026.
//
#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string>
#include <atomic>
#include <netdb.h>
#include <openssl/types.h>
#include <sys/epoll.h>

#include "../utils/ServerConfig.h"
#include "../network/connections/Connection.h"
#include "ThreadPool.h"

class WebServer {
private:
    SSL_CTX* ssl_ctx;

    int epollFd;

    int HttpListenSocket;
    int HttpsListenSocket;
    int ClientSocket;

    int sendResult;
    std::atomic<bool> isRunning;

    ServerConfig::Config serverConfig;
    void cleanupServer() const;
    void createListenSocket(int& ListenSocket, const std::string& port);

    //? IDE recommended
    [[nodiscard]] int createClientSocket(int socket) const;

    void createClientThread(std::unique_ptr<Connection> client);
    void consoleInput();

    void serveStatic(std::string &url, Connection &client);
    void connectionHandle(ThreadPool& pool, std::vector<epoll_event> &events);

    void addToEpoll(int socket);

public:
    WebServer(const std::string& path);
    ~WebServer();
    void startListen();
};
