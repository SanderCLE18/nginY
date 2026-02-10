//
// Created by Sande on 09.02.2026.
//
#include "FileHandler.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>

FileHandler::FileHandler() {}

FileHandler::~FileHandler() {

}
FileHandler::Response FileHandler::getSite(const std::string url) {
    Response response;

    std::ifstream file(url, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        response.found = false;
        response.header = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        return response;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    response.content.resize(size);
    file.read(response.content.data(), size);

    response.found = true;

    std::string filetype = getFileType(url);
    response.header = "HTTP/1.1 200 OK\r\n";
    response.header += "Content-Type: " + filetype + "\r\n";
    response.header += "Content-Length: " + std::to_string(size) + "\r\n";
    response.header += "Connection: close\r\n\r\n";

    file.close();
    return response;

}

std::string FileHandler::getUrlPath(std::string url) {
    if (url == "/") url = "/index.html";

    std::string urlPath = "www" + url;

    if (!std::filesystem::path(urlPath).has_extension()) {
        urlPath += ".html";
    }
    if (std::filesystem::exists(urlPath)) {
        return urlPath;
    }

    return "www/404.html";

}

FileHandler::Config FileHandler::parseConfig(const std::string& path) {
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
                        std::tuple<std::string, std::string> proxy = parseProxy(value);
                        rule.host = std::get<0>(proxy);
                        rule.port = std::get<1>(proxy);
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
std::tuple<std::string, std::string> FileHandler::parseProxy(const std::string value) {
    std::string target = value;
    std::string host;
    std::string port;
    if (target.ends_with(';')) target.pop_back();
    size_t start = target.find("://");

    if (start != std::string::npos) {
        target = target.substr(start+3);
    }
    size_t end = target.find(":");
    if (end != std::string::npos) {
        host = target.substr(0, end);
        port = target.substr(end+1);
    }
    else {
        //magic number
        port = 80;
        host = target;
    }
    return std::make_tuple(host, port);
}

std::string FileHandler::getFileType(std::string path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg")) return "image/jpeg";
    return "text/plain";
}