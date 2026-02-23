//
// Created by Sande on 09.02.2026.
//
#pragma once
#include <string>
#include <vector>

class FileHandler
{
private:
    static std::string getFileType(std::string path);
    static std::tuple<std::string, std::string> parseProxy(std::string value);

public:
    struct Response {
        std::string header;
        std::vector<char> content;
        bool found;
    };
    //Proxy rules!
    struct ProxyRules {
        std::string location;
        std::string host;
        std::string port;
        bool proxy;
    };

    struct Config {
        std::vector<FileHandler::ProxyRules> content;
        int portListen;
        bool found;
    };

    static Config parseConfig(const std::string& path);
    FileHandler();
    ~FileHandler();
    static Response getSite(const std::string &path);
    static std::string getUrlPath(std::string path);
};