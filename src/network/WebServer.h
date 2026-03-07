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

#include "../utilIO/ProxyConfig.h"
#include "connections/Connection.h"
#include "../ThreadPool.h"

class WebServer {
private:
    std::string ipAddress;
    SSL_CTX* ssl_ctx;

    int epollFd;

    int HttpListenSocket;
    int HttpsListenSocket;
    int ClientSocket;
    struct addrinfo* addrResult;
    struct addrinfo hints;

    int sendResult;
    std::atomic<bool> isRunning;

    ProxyConfig::Config proxyConfig;
    void cleanupServer() const;
    void resolveServer();
    void createListenSocket(int& ListenSocket);

    //? IDE recommended
    [[nodiscard]] int createClientSocket(int socket) const;

    void createClientThread(std::unique_ptr<Connection> client);
    void consoleInput();

    void serveStatic(std::string &url, Connection &client);
    void connectionHandle(ThreadPool& pool, std::vector<epoll_event> &events);

    void addToEpoll(int socket);

public:
    WebServer(std::string ipAddress, const std::string& path);
    ~WebServer();
    void startListen();
};
