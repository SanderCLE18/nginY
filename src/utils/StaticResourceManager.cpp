//
// Created by Sande on 09.02.2026.
//
#include "StaticResourceManager.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>

StaticResourceManager::StaticResourceManager() = default;

StaticResourceManager::~StaticResourceManager() = default;

StaticResourceManager::Response StaticResourceManager::getSite(const std::string &path) {
    Response response;

    std::ifstream file(path, std::ios::binary | std::ios::ate);

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

    std::string filetype = getFileType(path);
    response.header = "HTTP/1.1 200 OK\r\n";
    response.header += "Content-Type: " + filetype + "\r\n";
    response.header += "Content-Length: " + std::to_string(size) + "\r\n";
    response.header += "Connection: active\r\n\r\n";

    file.close();
    return response;

}

std::string StaticResourceManager::getUrlPath(std::string &url) {
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

std::string StaticResourceManager::getFileType(const std::string& path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg")) return "image/jpeg";
    return "text/plain";
}

long long StaticResourceManager::getContentLength(const std::string& header) {
    std::stringstream ss(header);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string lower = line;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (line.starts_with("content-length:")) {
            size_t after = line.find(":");
            std::string lengthString = line.substr(after + 1);
            lengthString.erase(std::remove_if(lengthString.begin(), lengthString.end(), ::isspace), lengthString.end());
            try {
                return std::stoll(lengthString);
            }catch (const std::exception&) {
                return -1;
            }
        }
    }
    return -1;

}