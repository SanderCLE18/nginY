//
// Created by Sande on 21.03.2026.
//
#include <string>

#include "network/connections/Connection.h"

class gTestConnectionParser {
    Connection& conn;
public:
    gTestConnectionParser(Connection& connection) : conn(connection) {}

    std::string readLine() {
        std::string line;
        char c;
        while (conn.read(&c, 1) > 0 && c != '\n') {
            line += c;
        }
        return line;
    }
};