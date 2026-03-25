//
// Created by Sande on 09.02.2026.
//
#include "WebServer.h"
#include "../utils/StaticResourceManager.h"
#include "ThreadPool.h"
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <atomic>
#include <thread>

#include "../network/ProxyConnection.h"
#include "../utils/Logger.h"
#include "../network/connections/Connection.h"
#include "../network/connections/HttpConnection.h"
#include "../network/connections/HttpsConnection.h"
#include "../network/socket/SocketFactory.h"

WebServer::WebServer(const std::string &pathConf, SocketFactory &factory) : serverConfig(
                                                                                ServerConfig::parseConfig(pathConf)),
                                                                            context(serverConfig) {
    try {
        HttpListenSocket = factory.createListenSocket(std::to_string(serverConfig.httpPortListen));
    } catch (std::exception &e) {
        Logger::log("Failed to create HTTP listen socket: " + std::string(e.what()), errno);
    }
    if (context.get() != nullptr) {
        try {
            HttpsListenSocket = factory.createListenSocket(std::to_string(serverConfig.httpsPortListen));
        } catch (std::exception &e) {
            std::cerr << "Failed to create HTTPS listen socket: " << e.what() << std::endl;
            HttpsListenSocket = -2;
        }
    } else {
        HttpsListenSocket = -2;
        std::cout << "HTTPS not supported" << std::endl;
    }
}

WebServer::~WebServer() {
    close(HttpsListenSocket);
    close(HttpListenSocket);
}

void WebServer::consoleInput() {
    std::string input;
    while (input != "exit") {
        std::getline(std::cin, input);
    }
    this->isRunning = false;
}

void WebServer::serveStatic(std::string &url, Connection &client) {
    std::string file = StaticResourceManager::getUrlPath(url);
    StaticResourceManager::Response response = StaticResourceManager::getSite(file);

    ssize_t staticResult = client.write(response.header.c_str(), response.header.size());
    if (staticResult == -1)
        Logger::log("Sending header failed", errno);

    if (response.found) {
        staticResult = client.write(response.content.data(), response.content.size());
        if (staticResult == -1)
            Logger::log("Sending content failed", errno);
    }
}

void WebServer::createClientThread(std::unique_ptr<Connection> client) {
    char recvbuf[8192];
    int clientResult = client->read(recvbuf, sizeof(recvbuf) - 1);
    if (clientResult > 0) {
        printf("Bytes received: %d\n", clientResult);
        //recvbuf[clientResult] = '\0';
        std::string request(recvbuf, clientResult);

        size_t first = request.find(' ');
        size_t second = request.find(' ', first + 1);

        if (first != std::string::npos && second != std::string::npos) {
            std::string url = request.substr(first + 1, second - first - 1);

            if (url.starts_with("/api/")) {
                ProxyConnection connection(*client, request, url, serverConfig);
            } else {
                serveStatic(url, *client);
            }
        }
    } else {
        Logger::log("Error with recv:", errno);
    }
    client->shutdown(SHUT_WR);
    client->close();
}

void WebServer::startListen(SocketFactory &factory) {
    //Logger::log("So based, I kneel...", 0);
    //listenToClients
    isRunning = true;
    std::thread t(&WebServer::consoleInput, this);

    ThreadPool pool(8);
    epollFd = epoll_create1(0);

    addToEpoll(HttpListenSocket);
    if (HttpsListenSocket != -2) addToEpoll(HttpsListenSocket);

    std::vector<epoll_event> events(64);
    do {
        connectionHandle(pool, events, factory);
    } while (isRunning.load());
    t.join();
}

void WebServer::addToEpoll(int socket) const {
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = socket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socket, &event) == -1) {
        Logger::log("Error adding socket to epoll:", errno);
    }
}

void WebServer::connectionHandle(ThreadPool &pool, std::vector<epoll_event> &events, SocketFactory &factory) {
    int n = epoll_wait(epollFd, events.data(), events.size(), -1);
    for (int i = 0; i < n; i++) {
        int fd = events[i].data.fd;
        int clientSocket = 0;

        if (fd == HttpListenSocket) {
            try {
                clientSocket = factory.createClientSocket(HttpListenSocket);
            } catch (std::exception &e) {
                Logger::log("Error creating client socket: ", errno);
            }

            if (clientSocket != -1) {
                pool.submit([this, clientSocket]() {
                    std::unique_ptr<Connection> conn = std::make_unique<HttpConnection>(clientSocket);
                    createClientThread(std::move(conn));
                });
            } else {
                int error = errno;
                if (error != EWOULDBLOCK) {
                    Logger::log("Error in main listening loop: ", error);
                }
            }
        }
        if (fd == HttpsListenSocket && HttpsListenSocket != -2) {
            try {
                clientSocket = factory.createClientSocket(HttpsListenSocket);
            } catch (const std::exception &e) {
                Logger::log("Faled to creating client socket", errno);
            }

            if (clientSocket != -1) {
                pool.submit([this, clientSocket]() {
                    try {
                        auto connection = std::make_unique<HttpsConnection>(clientSocket, context.get());
                        createClientThread(std::move(connection));
                    } catch (std::exception &e) {
                        Logger::log("Failed to create HTTPS connection: " + std::string(e.what()), errno);
                    }
                });
            } else {
                int error = errno;
                if (error != EWOULDBLOCK) {
                    Logger::log("Error in main listening loop: ", error);
                }
            }
        }
    }
}
