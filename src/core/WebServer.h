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

    /**
    * Console input for running "commands" ((just 'exit' is implemented (poorly)))  in the console while the server is running.
    */
    void consoleInput();

    void connectionHandle(ThreadPool &pool, std::vector<epoll_event> &events, SocketFactory &factory);

    /**
    *
    * @param socket socket
    */
    void addToEpoll(int socket) const;

public:
    WebServer(const std::string &path, SocketFactory &factory);

    ~WebServer();

    /**
    * "Main" loop for handling client connections.
    * Creates a thread pool and handles client connections using epoll.
    *
    * @param factory reference to a socket factory
    */
    void startListen(SocketFactory &factory);

    /**
     * Serves static files (.html, .css, .js, etc.) to client
     *
     * @param url reference to url string
     * @param client reference to client connection
     */
    void serveStatic(std::string &url, Connection &client);

    void createClientThread(std::unique_ptr<Connection> client);
};
