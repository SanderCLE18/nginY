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
    /**
     * @brief Server configuration
     */
    ServerConfig::Config serverConfig;

    /**
     * @brief Saved SSL context
     */
    SSLContext context;

    /**
     * @brief Epoll file descriptor
     */
    int epollFd;

    /**
     * @brief Socket designated for the HTTP server
     */
    int HttpListenSocket;

    /**
     * @brief Socket designated for the HTTPS server
     */
    int HttpsListenSocket;

    /**
     * @brief Atomic boolean flag indicating whether the server is running or not
     */
    std::atomic<bool> isRunning;

    /**
    * @brief Console input for running "commands" ((just 'exit' is implemented (poorly)))  in the console while the server is running.
    */
    void consoleInput();

    /**
     * @brief Delegates an incoming connection to a thread pool for handling
     * * When handled, the incoming conncetion will be sent to a client socket for further communication.
     *
     * @param pool Reference to thread pool
     * @param events Reference to epoll events
     * @param factory Reference to socket factory
     */
    void connectionHandle(ThreadPool &pool, std::vector<epoll_event> &events, SocketFactory &factory);

    /**
    * @brief Adds a socket to the epoll instance
    *
    * @param socket socket
    */
    void addToEpoll(int socket) const;

public:
    WebServer(const std::string &path, SocketFactory &factory);

    ~WebServer();

    /**
    * @brief "Main" loop for handling client connections.
    * * Creates a thread pool and handles client connections using epoll.
    *
    * @param factory reference to a socket factory
    */
    void startListen(SocketFactory &factory);

    /**
     * @brief Serves static files (.html, .css, .js, etc.) to client
     *
     * @param url reference to url string
     * @param client reference to client connection
     */
    void serveStatic(std::string &url, Connection &client);

    void createClientThread(std::unique_ptr<Connection> client);
};
