#include "../utils/Logger.h"
#include "../utils/ServerConfig.h"
#include "ProxyConnection.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

#include "../utils/StaticResourceManager.h"
#include "connections/Connection.h"
#include "socket/PosixSocketFactory.h"


ProxyConnection::ProxyConnection(Connection& client, std::string request, std::string url, const ServerConfig::VirtualHost& vhost) : client(client) {
	this->request = std::move(request);
	this->url = std::move(url);
	this->vhost = vhost;

	newConnection();
}

ProxyConnection::~ProxyConnection() {
	client.close();
}

void ProxyConnection::newConnection() {

	if (request.find(' ') == std::string::npos) return;
	const ServerConfig::ProxyRules* best = nullptr;
	for (const auto& proxy : vhost.content) {
		std::string loc = proxy.location;
		if (!loc.empty() && loc.back() == '/') {
			loc.pop_back();
		}
		if (url.starts_with(loc)) {
			if (best == nullptr || proxy.location.length() > best->location.length()) {
				best = &proxy;
			}
		}
	}
	if (best) {
		forwardRequest(best->host, best->port);
	}
	else {
		Logger::log("No matching configuration for URL: " + url, 1);
	}

}

void ProxyConnection::forwardRequest(const std::string& host, const std::string& port) {

	//Ultrataktisk factory.
	PosixSocketFactory posixSocketFactory;

	int backendSocket = posixSocketFactory.createListenSocket(host, port);

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

