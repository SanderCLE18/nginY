//
// Created by Sande on 09.02.2026.
//
#include "WebServer.h"
#include "FileHandler.h"
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


WebServer::WebServer(std::string ipAddress, int port) : ipAddress(std::move(ipAddress)), port(port){
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

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
	unsigned long mode = 1; x
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
//console function so program doesnt need to crash to exit - breaks cohesion
void WebServer::consoleInput() {
	std::string input;
	while (input != "exit"){
		std::getline(std::cin, input);
	}
	this->isRunning = false;
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

			std::string file = FileHandler::getUrlPath(url);
			FileHandler::Response response = FileHandler::getSite(file);
			ssize_t sendResult = send(client, response.header.c_str(), response.header.length(), 0);
			if (sendResult == -1) {
				std::printf("Sending header failed with error: %d\n", errno);

			}

			send(client, response.header.c_str(), response.header.length(), MSG_NOSIGNAL);
			if (response.found) {
				sendResult = send(client, response.content.data(), response.content.size(), 0);
				if (sendResult == -1) {
					std::printf("Sending content failed with error: %d\n", errno);

				}
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

	std::vector<std::thread> threads;
	for (int i = 0; i < 8; i++) {
		threads.push_back(std::thread(&WebServer::consoleInput, this));
	}

	do {
		ClientSocket = WebServer::createClientSocket();
		if (ClientSocket != -1) {
			std::thread newclient(&WebServer::createClientThread, this, ClientSocket);
			newclient.detach();
		}else {
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