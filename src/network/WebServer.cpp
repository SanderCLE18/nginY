//
// Created by Sande on 09.02.2026.
//
#include "WebServer.h"
#include "../FileHandler.h"
#include "../ThreadPool.h"
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <atomic>
#include <thread>

//Might need to update to have path to config ?!?!?
//Might need to clean up the constructor. This is ugly asl
WebServer::WebServer(std::string ipAddress, int port, const std::string &pathConf) : ipAddress(std::move(ipAddress)), port(port){
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

	proxyConfig = ProxyConfig::parseConfig(pathConf);

    resolveServer();
    createListenSocket();

}

WebServer::~WebServer() {
    close(ListenSocket);
}

void WebServer::resolveServer() {
    std::string temp = std::to_string(this->port);
    const char* str_port = temp.c_str();
    result = getaddrinfo(NULL, str_port, &hints, &addrResult);
    if (result != 0) {
        std::printf("getaddrinfo failed with error: %d\n", result);
        cleanupServer();
    }
}

void WebServer::cleanupServer() const{
    close(ListenSocket);
    close(ClientSocket);
    exit(1);
}

void WebServer::createListenSocket() {
	//set socket
	int opt = 1;
	setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	ListenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
	if (ListenSocket == -1) {
		std::printf("Error creating listening socket! %d\n", errno);
		cleanupServer();
	}
	//bind
	result = bind(ListenSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
	if (result == -1)	{
		std::printf("Error binding socket! %d\n", errno);
		freeaddrinfo(addrResult);
		cleanupServer();
	}
	freeaddrinfo(addrResult);
	//listen
	if (listen(ListenSocket, SOMAXCONN) == -1) {
		std::printf("Error listening: %d\n", errno);
		cleanupServer();
	}
	unsigned long mode = 1;
	ioctl(ListenSocket, FIONBIO, &mode);

}
int WebServer::createClientSocket() {
	//make client socket
	int Client;
	Client = accept(ListenSocket, NULL, NULL);
	if (Client == -1 && errno != EWOULDBLOCK) {
		std::printf("Accept failed: %d", errno);
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
//
void WebServer::serveProxy(const std::string& type, const std::string& url, const std::string& request, int client) {
	if (type == "GET") {
		for (const auto& proxy : proxyConfig.content) {
			if (proxy.location == url) {

				//Opens new socket for backend service.
				struct addrinfo hints, *res;
				memset(&hints, 0, sizeof(hints));
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM;

				int proxyAddr = getaddrinfo(proxy.host.c_str(), proxy.port.c_str(), &hints, &res);

				if (proxyAddr != 0) {
					std::printf("getaddrinfo failed with error: %d\n", proxyAddr);
					continue;
				}
				int backendSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
				if (backendSocket == -1) {
					std::printf("socket failed: %d\n", errno);
					freeaddrinfo(res);
					continue;
				}
				if (connect(backendSocket, res->ai_addr, res->ai_addrlen) == -1) {
					std::printf("connect failed: %d\n", errno);
					freeaddrinfo(res);
					close(backendSocket);
					continue;
				}

				freeaddrinfo(res);

				send(backendSocket, request.c_str(), request.length(), MSG_NOSIGNAL);

				// read response and forward to client
				char buffer[4096];
				ssize_t bytes;
				while ((bytes = recv(backendSocket, buffer, sizeof(buffer), 0)) > 0) {
					send(client, buffer, bytes, MSG_NOSIGNAL);
				}

				close(backendSocket);

			}
		}
	}
	else if (type == "POST") {
		for (const auto& proxy : proxyConfig.content) {
			if (proxy.location == url) {
				struct addrinfo hints, *res;
				memset(&hints, 0, sizeof(hints));

				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM;

				int proxyAddr = getaddrinfo(proxy.host.c_str(), proxy.port.c_str(),&hints, &res);
				if (proxyAddr != 0) {
					std::printf("getaddrinfo failed with error: %d\n", proxyAddr);
					continue;
				}
				int backendSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
				if (backendSocket == -1) {
					std::printf("socket failed: %d\n", errno);
					freeaddrinfo(res);
					continue;
				}

				freeaddrinfo(res);

				send(backendSocket, request.c_str(), request.length(), MSG_NOSIGNAL);
				char buffer[4096];
				size_t bytes;
				while ((bytes = recv(backendSocket, buffer, sizeof(buffer), 0)) > 0) {
					send(client, buffer, bytes, MSG_NOSIGNAL);
				}
				close(backendSocket);
			}
		}
	}
}
//Defining static as its own method. Think it makes the method createClientThread easier to read
void WebServer::serveStatic(const std::string& url, int client) {
	std::string file = FileHandler::getUrlPath(url);
	FileHandler::Response response = FileHandler::getSite(file);

	ssize_t result = send(client, response.header.c_str(), response.header.length(), MSG_NOSIGNAL);
	if (result == -1)
		std::printf("Sending header failed: %d\n", errno);

	if (response.found) {
		result = send(client, response.content.data(), response.content.size(), MSG_NOSIGNAL);
		if (result == -1)
			std::printf("Sending content failed: %d\n", errno);
	}
}
//Worker thread to handle multiple connections at once
void WebServer::createClientThread(int client) {
	unsigned long mode = 0; 
	ioctl(client, FIONBIO, &mode);

	char recvbuf[8192];
	int result = recv(client, recvbuf, sizeof(recvbuf) - 1, 0);
	if (result > 0) {
		printf("Bytes received: %d\n", result);
		std::string request(recvbuf);

		size_t first = request.find(" ");
		size_t second = request.find(" ", first + 1);

		if (first != std::string::npos && second != std::string::npos) {
			std::string url = request.substr(first + 1, second - first - 1);


			//Instead of having a routing table, Claude recommended just having this to begin with.
			if (url.starts_with("/api/")) {
				std::string type = request.substr(0, first);
				serveProxy(type, url, request, client);
			}
			else {
				serveStatic(url, client);
			}

		}
	}
	else {
		std::printf("Error with recv: %d\n", errno);

	}
	shutdown(client, SHUT_WR);
	close(client);

}

void WebServer::startListen() {
	//listenToClients
	isRunning = true;
	std::thread t(&WebServer::consoleInput, this);

	ThreadPool pool(8);

	do {
		ClientSocket = WebServer::createClientSocket();
		if (ClientSocket != -1) {
			pool.submit(&WebServer::createClientThread, this, ClientSocket);
		} else {
			int error = errno;
			if (error != EWOULDBLOCK) {
				std::printf("Error in main listening loop: %d\n", error);
			}
		}
		usleep(10000);
		
	} while (isRunning.load());
	t.join();
	
	result = shutdown(ClientSocket, SHUT_WR);
	if (result == -1) {
		cleanupServer();
	}
	cleanupServer();
}