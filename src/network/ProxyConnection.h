//
// Created by Sande on 09.02.2026.
//
#pragma once

#include "../utilIO/ProxyConfig.h"
#include <string>
#include <utility>

class ProxyConnection {

private:
	ProxyConfig::Config config;
	int client;
	std::string request;
	std::string url;

	void newConnection();
	void forwardRequest(const std::string& host, const std::string& port);

	int createSocket(const std::string& host, const std::string& port);




public:

	ProxyConnection(int client, std::string  request, std::string  url, const ProxyConfig::Config& config);

	~ProxyConnection();






};
