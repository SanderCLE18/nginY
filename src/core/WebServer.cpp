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
#include <set>
#include <thread>

#include "../network/ProxyConnection.h"
#include "../utils/Logger.h"
#include "../network/connections/Connection.h"
#include "../network/connections/HttpConnection.h"
#include "../network/connections/HttpsConnection.h"
#include "../network/socket/SocketFactory.h"

WebServer::WebServer(const std::string &pathConf, SocketFactory &factory) : serverConfig(
                                                                                ServerConfig::parseConfig(pathConf)),
                                                                            sniContext_(serverConfig) {
    try {
        std::set<int> httpSockets;
        for (auto& vhost : serverConfig.content) {
            for (auto& it : vhost.httpPort) {
                httpSockets.insert(it);
            }
        }

        for (auto& it : httpSockets) {
            httpListenSockets_.push_back(factory.createListenSocket(std::to_string(it)));
        }
    } catch (std::exception &e) {
        Logger::log("Failed to create HTTP listen socket: " + std::string(e.what()), errno);
    }
    if (sniContext_.getDefault() != nullptr) {
        try {
            std::set<int> httpsSockets;
            for (auto& vhost : serverConfig.content) {
                for (auto& it : vhost.httpsPort) {
                    httpsSockets.insert(it);
                }
            }

            for (auto& it : httpsSockets) {
                httpsListenSockets_.push_back(factory.createListenSocket(std::to_string(it)));
            }
        } catch (std::exception &e) {
            Logger::log("Failed to create HTTPS listen socket: " + std::string(e.what()), errno);
        }
    } else {
        std::cout << "HTTPS not supported" << std::endl;
    }
}

WebServer::~WebServer() {
    for (int port : httpListenSockets_) {
        close(port);
    }
    for (int port : httpsListenSockets_) {
        close(port);
    }
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

void WebServer::createHttpClientThread(std::unique_ptr<Connection> client) {
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
            size_t hostPos = request.find("Host: ");
            if (hostPos == std::string::npos) {
                Logger::log("No Host header in request", 1);
                return;
            }
            size_t nextPos = request.find("\r\n", hostPos);
            std::string host = request.substr(hostPos + 6, nextPos - (hostPos + 6) );
            for (const auto& it : serverConfig.content) {
                if (it.hostName == host && !it.httpsPort.empty()) {
                    std::string response = "HTTP/1.1 301 Moved Permanently\r\nLocation: https://";
                    response.append(host);
                    response.append(url);
                    response.append("\r\nContent-Length: 0\r\n\r\n");
                    client->write(response.c_str(), response.size());
                    return;
                }
                if (it.hostName == host) {
                    ProxyConnection connection(*client, request, url, it);
                }
            }
        }
    } else {
        Logger::log("Error with recv:", errno);
    }
    client->shutdown(SHUT_WR);
    client->close();
}

void WebServer::createHttpsClientThread(std::unique_ptr<Connection> client) {
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
            size_t hostPos = request.find("Host: ");
            if (hostPos == std::string::npos) {
                Logger::log("No Host header in request", 1);
                return;
            }
            size_t nextPos = request.find("\r\n", hostPos);
            std::string host = request.substr(hostPos + 6, nextPos - (hostPos + 6) );
            for (const auto& it : serverConfig.content) {
                if (it.hostName == host) {
                    ProxyConnection connection(*client, request, url, it);
                }
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

    for (auto& it : httpListenSockets_) {
        addToEpoll(it);
    }
    if (!httpsListenSockets_.empty()) {
        for (auto& it : httpsListenSockets_) {
            addToEpoll(it);
        }
    }

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

void WebServer::acceptAndSubmit(ThreadPool &pool, SocketFactory &factory, int fd,
                                std::function<void(int)> handler) {
    int clientSocket = 0;
    try {
        clientSocket = factory.createClientSocket(fd);
    } catch (std::exception &e) {
        Logger::log("Error creating client socket: " + std::string(e.what()), errno);
    }

    if (clientSocket != -1) {
        pool.submit([handler = std::move(handler), clientSocket]() {
            handler(clientSocket);
        });
    } else {
        int error = errno;
        if (error != EWOULDBLOCK) {
            Logger::log("Error in main listening loop: ", error);
        }
    }
}

void WebServer::connectionHandle(ThreadPool &pool, std::vector<epoll_event> &events, SocketFactory &factory) {
    int n = epoll_wait(epollFd, events.data(), events.size(), -1);
    for (int i = 0; i < n; i++) {
        int fd = events[i].data.fd;

        if (std::ranges::contains(httpListenSockets_, fd)) {
            acceptAndSubmit(pool, factory, fd, [this](int sock) {
                createHttpClientThread(std::make_unique<HttpConnection>(sock));
            });
        }
        if (std::ranges::contains(httpsListenSockets_, fd)) {
            acceptAndSubmit(pool, factory, fd, [this](int sock) {
                try {
                    createHttpsClientThread(std::make_unique<HttpsConnection>(sock, sniContext_.getDefault()));
                } catch (std::exception &e) {
                    Logger::log("Failed to create HTTPS connection: " + std::string(e.what()), errno);
                }
            });
        }
    }
}
