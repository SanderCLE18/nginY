//
// Created by Sande on 09.02.2026.
//
#include "WebServer.h"
#include "../utils/StaticResourceManager.h"
#include "../utils/Logger.h"

int main() {

    std::string configPath = "config.conf";
    WebServer server(configPath);

    return 0;

}
