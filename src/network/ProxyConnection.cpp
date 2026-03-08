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

#include "../utils/StaticResourceManager.h"
#include "connections/Connection.h"



ProxyConnection::ProxyConnection(Connection& client, std::string request, std::string url, const ServerConfig::Config& config) : client(client) {
	this->request = std::move(request);
	this->url = std::move(url);
	this->config = config;

	newConnection();
}

ProxyConnection::~ProxyConnection() {
	client.close();
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

	if (backendSocket == -1) return;

	size_t headerPos = request.find("\r\n\r\n");

	if (headerPos == std::string::npos) {
		Logger::log("Error: Failed to find header in request.", errno);
		close(backendSocket);
		return;
	}

	//Split
	std::string header = request.substr(0, headerPos + 4);
	size_t contentLength = StaticResourceManager::getContentLength(header);


	send(backendSocket, header.c_str(), header.length(), MSG_NOSIGNAL);

	if (contentLength > 0 ) {
		size_t recieved = request.length() - (headerPos + 4);
		if (recieved > 0) {
			send(backendSocket, request.c_str() + headerPos + 4, recieved, MSG_NOSIGNAL);
		}

		size_t remaining = contentLength - recieved;
		constexpr size_t CHUNK = 64*1024;
		std::vector<char> buf(CHUNK);

		while (remaining > 0) {
			size_t read = std::min(remaining, CHUNK);
			ssize_t n = client.read(buf.data(), read);
			if (n<=0) break;
			send(backendSocket, buf.data(), n, MSG_NOSIGNAL);
			remaining -= n;
		}

	}


	char buffer[4096];
	size_t bytes;
	while ((bytes = recv(backendSocket, buffer, sizeof(buffer), 0)) > 0) {
		client.write(buffer, bytes);
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
		return -1;
	}
	int backendSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (backendSocket == -1) {
		Logger::log("Error: Failed to create backend socket: ", errno);
		freeaddrinfo(res);
		return -1;
	}
	if (connect(backendSocket, res->ai_addr, res->ai_addrlen) == -1) {
		Logger::log("Error: Failed to connect to backend: ", errno);
		freeaddrinfo(res);
		close(backendSocket);
		return -1;
	}

	freeaddrinfo(res);
	return backendSocket;
}
