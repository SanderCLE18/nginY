#include "../utils/Logger.h"
#include "ProxyConnection.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <cerrno>
#include <cstring>


ProxyConnection::ProxyConnection(int client, std::string  request, std::string  url, const ProxyConfig::Config& config) {
    this->client  = client;
    this->request = std::move(request);
    this->url = std::move(url);
    this->config = config;

	this->newConnection();
}

ProxyConnection::~ProxyConnection() {
	close(client);
}

void ProxyConnection::newConnection() {
	size_t firstSpace = request.find(" ");
	if (firstSpace == std::string::npos) return;

	std::string type = request.substr(0, firstSpace);

	for (const auto& proxy : config.content) {
		if (proxy.location == url) {
			forwardRequest(proxy.host, proxy.port);
			return;
		}
	}
	Logger::log("No matching configuration for URL: " + url, 1);
}

void ProxyConnection::forwardRequest(const std::string& host, const std::string& port) {

	int backendSocket = createSocket(host, port);

	if (backendSocket == -1) {
		close(backendSocket);
		return;
	}

	send(backendSocket, request.c_str(), request.length(), MSG_NOSIGNAL);
	char buffer[4096];
	size_t bytes;
	while ((bytes = recv(backendSocket, buffer, sizeof(buffer), 0)) > 0) {
		send(client, buffer, bytes, MSG_NOSIGNAL);
	}

	close(backendSocket);
}

int ProxyConnection::createSocket(const std::string& host, const std::string& port) {
	//Opens new socket for backend service.
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int proxyAddr = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);

	if (proxyAddr != 0) {
		Logger::log("Error: Failed to get address info: ", proxyAddr);
	}
	int backendSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (backendSocket == -1) {
		Logger::log("Error: Failed to create backend socket: ", errno);
		freeaddrinfo(res);
	}
	if (connect(backendSocket, res->ai_addr, res->ai_addrlen) == -1) {
		Logger::log("Error: Failed to connect to backend: ", errno);
		freeaddrinfo(res);
		close(backendSocket);
	}

	freeaddrinfo(res);
	return backendSocket;
}
