//
// Created by Sande on 20.06.2026.
//
#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "SSLContext.h"

class SNIContext {
private:
    std::unordered_map<std::string, std::unique_ptr<SSLContext>> contexts;

public:
    SNIContext(const ServerConfig::Config& config);
    SSL_CTX* getDefault() const;
private:
    static int sniCallback(SSL* ssl, int*, void* arg);

};