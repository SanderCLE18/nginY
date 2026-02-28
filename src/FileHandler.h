//
// Created by Sande on 09.02.2026.
//
#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

class FileHandler
{
private:
    static std::string getFileType(const std::string& path);

public:
    struct Response {
        std::string header;
        std::vector<char> content;
        bool found;
    };

    FileHandler();
    ~FileHandler();
    static Response getSite(const std::string &path);
    static std::string getUrlPath(std::string &path);

    static long long getContentLength(const std::string& header);

};