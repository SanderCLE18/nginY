//
// Created by Sande on 09.02.2026.
//
#include <filesystem>

#include "WebServer.h"
#include "../network/socket/PosixSocketFactory.h"
#include "../utils/StaticResourceManager.h"

int main() {
    std::string configPath = "config.conf";
    PosixSocketFactory posixFactory;
    WebServer server(configPath, posixFactory);
    server.startListen(posixFactory);

    return 0;
}
