//
// Created by Sande on 08.03.2026.
//

#include "FileReader.h"
#include <fstream>

std::string FileReader::getPassword(const std::string &path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Failed to open password file");

    std::string password;
    std::getline(f, password);
    return password;

}
