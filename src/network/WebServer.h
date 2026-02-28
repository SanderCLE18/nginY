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

#include "../utilIO/ProxyConfig.h"

class WebServer {
private:
    std::string ipAddress;
    int port;

    int result;
    int ListenSocket;
    int ClientSocket;
    struct addrinfo* addrResult;
    struct addrinfo hints;

    int sendResult;
    std::atomic<bool> isRunning;

    ProxyConfig::Config proxyConfig;

    void cleanupServer() const;
    void resolveServer();
    void createListenSocket();

    //? IDE recommended
    [[nodiscard]] int createClientSocket() const;

    int createBackendConnection();
    void createClientThread(int client);
    void consoleInput();

    void serveStatic(std::string &url, int client);
    void serveProxy(const std::string& type, const std::string& url, const std::string& request, int client);

public:
    WebServer(std::string ipAddress, int port, const std::string& path);
    ~WebServer();
    void startListen();
};
