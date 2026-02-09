//
// Created by Sande on 09.02.2026.
//
#include "FileHandler.h"
#include <string>
#include <fstream>
#include <filesystem>

FileHandler::FileHandler() {}

FileHandler::~FileHandler() {

}
FileHandler::Response FileHandler::getSite(std::string url) {
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

std::string FileHandler::getFileType(std::string path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg")) return "image/jpeg";
    return "text/plain";
}