//
// Created by Sande on 08.03.2026.
//

#pragma once
#include <string>

/**
 * FileReader class provides functionality to read passwords from files.
 */
class FileReader {
public:
    /**
     * @brief returns a password from a given file.
     *
     * @param path reference path to the password file
     * @return read password from the file
     */
    static std::string getPassword(const std::string& path);
};