//
// Created by Sande on 08.03.2026.
//

#include "SSLContext.h"
#include "../utils/FileReader.h"
#include "../utils/ServerConfig.h"


SSLContext::SSLContext(const ServerConfig::Config& config) {
    if (config.keyPath.empty() || config.certPath.empty() ) {
        ctx = nullptr;
        return;
    }
    ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) throw std::runtime_error("Failed to create SSL context");

    if (SSL_CTX_use_certificate_file(ctx, config.certPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("Failed to load certificate");
    }

    std::string password = FileReader::getPassword(config.passPath);

    SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)password.c_str());
    SSL_CTX_set_default_passwd_cb(ctx, [](char* buf, int size, int, void* userdata) -> int {
       const char* pwd = static_cast<const char*>(userdata);
        strncpy(buf, pwd, size);
        buf[size - 1] = '\0';
        return strlen(buf);
    });


    if (SSL_CTX_use_PrivateKey_file(ctx, config.keyPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("Failed to load private key");
    }
}