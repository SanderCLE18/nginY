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

//Might need to update to have path to config ?!?!?
//Might need to clean up the constructor. This is ugly asl
WebServer::WebServer(const std::string &pathConf) {


	serverConfig = ServerConfig::parseConfig(pathConf);

	createListenSocket(HttpListenSocket, "80");
    createListenSocket(HttpsListenSocket, "443");

}

WebServer::~WebServer() {
    close(HttpsListenSocket);
	close(HttpListenSocket);
}


void WebServer::cleanupServer() const{
    close(HttpsListenSocket);
	close(HttpListenSocket);
    close(ClientSocket);
    exit(1);
}

void WebServer::createListenSocket(int& ListenSocket,const std::string& port) {
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int result = getaddrinfo(NULL, port.c_str(), &hints, &res);
	if (result != 0) {
		Logger::log("getaddrinfo failed:", result);
		cleanupServer();
		return (void)0;
	}
	//set socket
	int opt = 1;
	ListenSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ListenSocket == -1) {
		Logger::log("Error: Failed to create listening socket: ", errno);
		cleanupServer();
		return (void)0;
	}
	//bind
	int listenResult = bind(ListenSocket, res->ai_addr, (int)res->ai_addrlen);
	if (listenResult == -1)	{
		Logger::log("Error: Failed to bind listening socket: ", errno);
		freeaddrinfo(res);
		cleanupServer();
		return (void)0;
	}
	freeaddrinfo(res);
	//listen
	if (listen(ListenSocket, SOMAXCONN) == -1) {
		Logger::log("Error: Failed to listen on socket: ", errno);
		cleanupServer();
		return (void)0;
	}
	unsigned long mode = 1;
	ioctl(ListenSocket, FIONBIO, &mode);

}
int WebServer::createClientSocket(int socket) const {
	//make client socket
	int Client;
	Client = accept(socket, NULL, NULL);
	if (Client == -1 && errno != EWOULDBLOCK) {
		Logger::log("Error: Failed to accept incoming connection: ", errno);
	}
	return Client;
}
//console function so program doesnt need to crash to exit
void WebServer::consoleInput() {
	std::string input;
	while (input != "exit"){
		std::getline(std::cin, input);
	}
	this->isRunning = false;
}

//Defining static as its own method. Think it makes the method createClientThread easier to read
void WebServer::serveStatic(std::string &url, Connection& client) {
	std::string file = StaticResourceManager::getUrlPath(url);
	StaticResourceManager::Response response = StaticResourceManager::getSite(file);

	//content is entire string
	ssize_t staticResult = client.write(response.content.data(), response.content.size());
	if (staticResult == -1)
		Logger::log("Sending header failed", errno);

	if (response.found) {
		staticResult = client.write(response.content.data(), response.content.size());
		if (staticResult == -1)
			Logger::log("Sending content failed", errno);
	}
}
//Worker thread to handle multiple connections at once
void WebServer::createClientThread(std::unique_ptr<Connection> client) {
	char recvbuf[8192];
	int clientResult = client->read(recvbuf, sizeof(recvbuf) - 1);
	if (clientResult > 0) {
		printf("Bytes received: %d\n", clientResult);
		std::string request(recvbuf);

		size_t first = request.find(" ");
		size_t second = request.find(" ", first + 1);

		if (first != std::string::npos && second != std::string::npos) {
			std::string url = request.substr(first + 1, second - first - 1);


			//Instead of having a routing table, Claude recommended just having this to begin with.
			if (url.starts_with("/api/")) {
				ProxyConnection connection(*client, request, url, serverConfig);

			}
			else {
				serveStatic(url, *client);
			}

		}
	}
	else {
		Logger::log("Error with recv:", errno);
	}
	client->shutdown(SHUT_WR);
	client->close();

}

void WebServer::startListen() {
	Logger::log("So based, I kneel...", 0);
	//listenToClients
	isRunning = true;
	std::thread t(&WebServer::consoleInput, this);

	ThreadPool pool(8);
	epollFd = epoll_create1(0);

	addToEpoll(HttpListenSocket);
	addToEpoll(HttpsListenSocket);

	std::vector<epoll_event> events(64);

	do {
		connectionHandle(pool, events);
	} while (isRunning.load());
	t.join();
	
	int listenResult = shutdown(ClientSocket, SHUT_WR);
	if (listenResult == -1) {
		cleanupServer();
	}
	cleanupServer();
}

void WebServer::addToEpoll(int socket) {
	epoll_event event{};
	event.events = EPOLLIN;
	event.data.fd = socket;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socket, &event) == -1) {
		Logger::log("Error adding socket to epoll:", errno);
	}
}

void WebServer::connectionHandle(ThreadPool &pool, std::vector<epoll_event> &events) {
	int n = epoll_wait(epollFd, events.data(), events.size(), -1);
	for (int i = 0; i < n; i++) {
		int fd = events[i].data.fd;

		if (fd == HttpListenSocket) {
			int clientSocket = WebServer::createClientSocket(HttpListenSocket);
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
		if (fd == HttpsListenSocket) {
			int clientSocket = WebServer::createClientSocket(HttpsListenSocket);
			if (clientSocket != -1) {
				auto connection = std::make_unique<HttpsConnection>(clientSocket, ssl_ctx);
				pool.submit(&WebServer::createClientThread, this, std::move(connection));
			} else {
				int error = errno;
				if (error != EWOULDBLOCK) {
					Logger::log("Error in main listening loop: ", error);
				}
			}
		}
	}
}

