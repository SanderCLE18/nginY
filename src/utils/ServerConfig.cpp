//
// Created by Sande on 25.02.2026.
//
#include "ServerConfig.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "Logger.h"

ServerConfig::Config ServerConfig::parseConfig(const std::string& path) {
    Config config;

    std::ifstream file(path);

    if (!file.is_open()) {
        config.httpPortListen = 8080;
        config.httpsPortListen = 8443;
        config.found = false;
        Logger::log("Config file not found", 1);
        return config;
    }

    config.found = true;
    std::string line, word;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        word.clear();
        std::stringstream ss(line);
        ss >> word;
        if (word == "server") {
            config.content.push_back(parseVirtualHost(file));
        }

    }
    file.close();
    return config;
}

ServerConfig::VirtualHost ServerConfig::parseVirtualHost(std::ifstream& file) {
    VirtualHost host;
    std::string line;

    while (std::getline(file, line) && line.find('}') == std::string::npos) {
        std::stringstream ss(line);
        std::string word;
        ss >> word;

        auto readValue = [&](std::string& field) {
            ss >> field;
            if (!field.empty() && field.back() == ';')
                field.pop_back();
        };

        if (word == "ssl_password_file") {
            readValue(host.passPath);
        }
        else if (word == "ssl_certificate") {
            readValue(host.certPath);
        }
        else if (word == "ssl_certificate_key") {
            readValue(host.keyPath);
        }
        else if (word == "serverName") {
            ss >> host.hostName;
        }
        else if (word == "location") {
            std::cout << "found location" << std::endl;
            ProxyRules rule;
            ss >> rule.location;
            while (std::getline(file, line) && line.find('}') == std::string::npos) {
                std::stringstream block(line);

                std::string key, value;
                block >> key >> value;

                if (key == "proxy_pass") {
                    auto [host,port] = parseProxy(value);
                    rule.host = host;
                    rule.port = port;
                }
                else if (key == "root") {
                    rule.proxy = false;
                }
            }
            host.content.push_back(rule);
        }
    }
    return host;
}

std::tuple<std::string, std::string> ServerConfig::parseProxy(const std::string& value) {
    std::string target = value;
    std::string host;
    std::string port;
    if (target.ends_with(';')) target.pop_back();
    size_t start = target.find("://");

    if (start != std::string::npos) {
        target = target.substr(start+3);
    }
    size_t end = target.find(':');
    if (end != std::string::npos) {
        host = target.substr(0, end);
        port = target.substr(end+1);
    }
    else {
        //magic number
        port = "80";
        host = target;
    }
    return std::make_tuple(host, port);
}