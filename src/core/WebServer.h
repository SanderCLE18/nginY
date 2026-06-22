//
// Created by Sande on 09.02.2026.
//
#pragma once
#include <string>
#include <atomic>
#include <functional>
#include <sys/epoll.h>

#include "../utils/ServerConfig.h"
#include "../network/connections/Connection.h"
#include "ThreadPool.h"
#include "../network/SNIContext.h"
#include "../network/socket/SocketFactory.h"

class WebServer {
public:
    /**
     * @brief Holds the host and URL parsed from an incoming HTTP request.
     */
    struct ParsedRequest {
        std::string host; ///< Value of the Host header
        std::string url;  ///< Request URL extracted from the request line
    };
private:
    /**
     * @brief Server configuration
     */
    ServerConfig::Config serverConfig;

    /**
     * @brief Saved SSL context
     */
    SNIContext sniContext_;

    /**
     * @brief Epoll file descriptor
     */
    int epollFd = 0;

    /**
     * @brief Socket designated for the HTTP server
     */
    std::vector<int> httpListenSockets_;

    /**
     * @brief Socket designated for the HTTPS server
     */
    std::vector<int> httpsListenSockets_;

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

    /**
     * @brief Accepts a client socket and submits it to the thread pool.
     *
     * @param pool Reference to thread pool
     * @param factory Reference to socket factory
     * @param fd The listening socket fd that triggered the epoll event
     * @param handler Function that takes the accepted socket fd and handles the connection
     */
    void acceptAndSubmit(ThreadPool &pool, SocketFactory &factory, int fd,
                         std::function<void(int)> handler);

    /**
     * @brief Parses the host and URL from a raw HTTP request string.
     *
     * @param request The raw HTTP request string received from the client
     * @return ParsedRequest containing the Host header value and request URL
     */
    ParsedRequest parseRequest(const std::string& request);

    /**
     * @brief Finds the matching virtual host and forwards the request via ProxyConnection.
     *
     * @param parsedRequest The parsed host and URL from the request
     * @param client The client connection to forward responses to
     * @param request The full raw HTTP request string
     */
    void dispatchToVhost(const ParsedRequest &parsedRequest, std::unique_ptr<Connection> client, std::string &request);

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

    /**
     * @brief Reads and handles an incoming HTTP connection.
     * Redirects to HTTPS if the matched virtual host has HTTPS ports configured.
     *
     * @param client The accepted HTTP client connection
     */
    void createHttpClientThread(std::unique_ptr<Connection> client);

    /**
     * @brief Reads and handles an incoming HTTPS connection.
     *
     * @param client The accepted HTTPS client connection
     */
    void createHttpsClientThread(std::unique_ptr<Connection> client);


};
