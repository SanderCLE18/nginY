//
// Created by Sande on 25.02.2026.
//

#pragma once
#include <string>
#include <vector>

class ServerConfig {
private:
    static std::tuple<std::string, std::string> parseProxy(const std::string& value);

public:
    //Proxy rules!
    struct ProxyRules {
        std::string location;
        std::string host;
        std::string port;

        bool proxy;
    };

    struct Config {
        std::vector<ServerConfig::ProxyRules> content;
        int httpsPortListen;
        int httpPortListen;
        bool found;

        std::string passPath;
        std::string certPath;
        std::string keyPath;
    };

    static Config parseConfig(const std::string& path);
};

