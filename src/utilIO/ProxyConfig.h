//
// Created by Sande on 25.02.2026.
//

#pragma once
#include <string>
#include <vector>

class ProxyConfig {
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
        std::vector<ProxyConfig::ProxyRules> content;
        int portListen;
        bool found;
    };

    static Config parseConfig(const std::string& path);
};

