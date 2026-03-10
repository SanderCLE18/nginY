//
// Created by Sande on 09.02.2026.
//
#include <filesystem>
#include <iostream>

#include "WebServer.h"
#include "../utils/StaticResourceManager.h"

int main() {

    std::string configPath = "config.conf";
    WebServer server(configPath);
    server.startListen();

    std::cout<<"somefingwrong"<<std::endl;

    return 0;

}
