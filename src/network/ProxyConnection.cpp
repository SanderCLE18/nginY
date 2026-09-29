#include "../utils/Logger.h"
#include "../utils/ServerConfig.h"
#include "ProxyConnection.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <sstream>
#include <algorithm>

#include "../utils/StaticResourceManager.h"
#include "connections/Connection.h"
#include "socket/PosixSocketFactory.h"

static const std::string gatewayTimeout =
			"HTTP/1.1 504 Gateway Timeout\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 16\r\n"
			"Connection: close\r\n\r\n"
			"Gateway Timeout\n";

static const std::string badGateway =
			"HTTP/1.1 502 Bad Gateway\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 12\r\n"
			"Connection: close\r\n\r\n"
			"Bad Gateway\n";

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
		if (best->proxy) {
			forwardRequest(best->host, best->port);
		}
		else {
			std::string file = StaticResourceManager::getUrlPath(url);
			StaticResourceManager::Response response = StaticResourceManager::getSite(file);
			client.write(response.header.c_str(), response.header.size());
			if (response.found) {
				client.write(response.content.data(), response.content.size());
			}

		}

	}
	else {
		Logger::log("No matching configuration for URL: " + url, 1);
	}

}

void ProxyConnection::forwardRequest(const std::string& host, const std::string& port) {

	//Ultrataktisk factory.
	PosixSocketFactory posixSocketFactory;

	int backendSocket = -1;
	try {
		backendSocket = posixSocketFactory.connectSocket(host, port);
	}
	catch (std::exception& e) {
		Logger::log("Could not connect to backend", errno);
	}
	if (backendSocket == -1) {
		Logger::log("Could not connect to backend:" + host +":"+port + "errno:", errno);

		client.write(badGateway.c_str(), badGateway.size());
		return;
	}

	size_t headerPos = request.find("\r\n\r\n");

	if (headerPos == std::string::npos) {
		Logger::log("Error: Failed to find header in request.", errno);
		close(backendSocket);
		return;
	}

	timeval tv{.tv_sec=30, .tv_usec=0};
	setsockopt(backendSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	//Split
	std::string header = request.substr(0, headerPos + 4);
	long long contentLength = StaticResourceManager::getContentLength(header);

	std::string forward;
	std::istringstream in(header.substr(0, headerPos));
	std::string line;
	while (std::getline(in, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		std::string low = line;
		std::ranges::transform(low, low.begin(), ::tolower);

		if (low.starts_with("connection:") || low.starts_with("keep-alive:")) {
			continue;
		}
		forward += line + "\r\n";
	}
	forward += "Connection: close\r\n\r\n";
	send(backendSocket, forward.c_str(), forward.length(), MSG_NOSIGNAL);

	if (contentLength > 0 ) {
		size_t received = request.length() - (headerPos + 4);
		if (received > 0) {
			send(backendSocket, request.c_str() + headerPos + 4, received, MSG_NOSIGNAL);
		}
		
		size_t remaining = received < static_cast<size_t>(contentLength) ? static_cast<size_t>(contentLength) - received : 0;
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
	ssize_t bytes;
	ssize_t sent = 0;
	while ((bytes = recv(backendSocket, buffer, sizeof(buffer), 0)) > 0) {
		client.write(buffer, bytes);
		sent += bytes;
	}
	int error = errno;
	if (bytes == -1) {
		bool timeout = error == EAGAIN || error == EWOULDBLOCK;
		if (sent == 0) {
			const std::string& reps = timeout ? gatewayTimeout : badGateway;
			client.write(reps.c_str(), reps.length());
		}
		Logger::log(timeout ? "Backend timed out, errno: " : "Backend recv failed, errno:", error);
	}

	close(backendSocket);
}

