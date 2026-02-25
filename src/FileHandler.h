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

public:
    struct Response {
        std::string header;
        std::vector<char> content;
        bool found;
    };

    FileHandler();
    ~FileHandler();
    static Response getSite(const std::string &path);
    static std::string getUrlPath(std::string path);
};