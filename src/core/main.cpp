//
// Created by Sande on 09.02.2026.
//
#include <filesystem>

#include "WebServer.h"
#include "../utils/StaticResourceManager.h"

int main() {

    std::string configPath = "config.conf";
    WebServer server(configPath);
    server.startListen();


    return 0;

}
