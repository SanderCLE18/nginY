//
// Created by Sande on 09.02.2026.
//
#pragma once

#include "../utils/ServerConfig.h"
#include <string>
#include <utility>

#include "connections/Connection.h"

class ProxyConnection {

private:
	ServerConfig::Config config;
	Connection& client;
	std::string request;
	std::string url;

	void newConnection();
	void forwardRequest(const std::string& host, const std::string& port);

	int createSocket(const std::string& host, const std::string& port);

public:

	ProxyConnection(Connection& client, std::string  request, std::string  url, const ServerConfig::Config& config);

	~ProxyConnection();

};
