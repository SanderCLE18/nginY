//
// Created by Sande on 09.02.2026.
//
#pragma once
#include <string>
#include <atomic>
#include <sys/epoll.h>

#include "../utils/ServerConfig.h"
#include "../network/connections/Connection.h"
#include "ThreadPool.h"
#include "../network/SSLContext.h"
#include "../network/socket/SocketFactory.h"

class WebServer {
private:
    ServerConfig::Config serverConfig;
    SSLContext context;

    int epollFd;

    int HttpListenSocket;
    int HttpsListenSocket;

    int sendResult;
    std::atomic<bool> isRunning;


    void consoleInput();
    void connectionHandle(ThreadPool& pool, std::vector<epoll_event> &events, SocketFactory &factory);

    void addToEpoll(int socket) const;

public:
    WebServer(const std::string& path, SocketFactory& factory);
    ~WebServer();
    void startListen(SocketFactory &factory);

    void serveStatic(std::string &url, Connection &client);
    void createClientThread(std::unique_ptr<Connection> client);
};
