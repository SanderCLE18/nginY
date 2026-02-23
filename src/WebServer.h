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

    void cleanupServer() const;
    void resolveServer();
    void createListenSocket();
    int createClientSocket();
    int createBackendConnection();
    void createClientThread(int client);
    void consoleInput();


public:
    WebServer(std::string ipAddress, int port);
    ~WebServer();
    void startListen();
};
