//
// Created by Sande on 09.02.2026.
//
#pragma once

#include "../utils/ServerConfig.h"
#include <string>
#include <utility>

#include "connections/Connection.h"

/**
 * @brief ProxyConnection class handles the connection and request forwarding for the proxy server.
 */
class ProxyConnection {

private:

	/**
	 * @brief Stored configuration for the proxy server.
	 */
	ServerConfig::Config config;

	/**
	 * @brief Reference to the client connection.
	 */
	Connection& client;

	/**
	 * @brief Request string received from the client.
	 */
	std::string request;

	/**
	 * @brief URL extracted from the request.
	 */
	std::string url;

	/**
	 * @brief checks if the incoming request is valid and can be processed.
	 * Forwards request if valid.
	 */
	void newConnection();

	/**
	 * @brief Forwards the request to the specified host and port.
	 *
	 * @param host The back-end host to forward the request to.
	 * @param port The port to forward the request to.
	 */
	void forwardRequest(const std::string& host, const std::string& port);


public:
	/**
	 * @brief Creates a new ProxyConnection instance.
	 *
	 * @param client Reference to the client connection.
	 * @param request the full request string received from the client.
	 * @param url The destination of the request.
	 * @param config The configuration for the proxy server.
	 */
	ProxyConnection(Connection& client, std::string request, std::string url, const ServerConfig::Config& config);

	/**
	 * @brief Destructor for ProxyConnection.
	 */
	~ProxyConnection();

};
