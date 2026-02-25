//
// Created by Sande on 25.02.2026.
//
#include "ProxyConfig.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>

ProxyConfig::Config ProxyConfig::parseConfig(const std::string& path) {
    Config config;

    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        config.portListen = 80;
        config.found = false;
        return config;
    }
    else {
        std::string line, word;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            ss >> word;
            if (word == "listen") {
                ss >> config.portListen;
            }
            else if (word == "location") {
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
                config.content.push_back(rule);
            }
        }
    }
    file.close();
    return config;
}
std::tuple<std::string, std::string> ProxyConfig::parseProxy(const std::string& value) {
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