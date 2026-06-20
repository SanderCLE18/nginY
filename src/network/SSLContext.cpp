//
// Created by Sande on 08.03.2026.
//

#include "SSLContext.h"
#include "../utils/FileReader.h"
#include "../utils/ServerConfig.h"
#include "../utils/Logger.h"


SSLContext::SSLContext(const ServerConfig::VirtualHost& host) {
    if (host.keyPath.empty() || host.certPath.empty() ) {
        Logger::log("SSL context creation failed: missing key or certificate path: ", 1);
        ctx = nullptr;
        return;
    }
    ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) throw std::runtime_error("Failed to create SSL context");

    if (SSL_CTX_use_certificate_file(ctx, host.certPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("Failed to load certificate");
    }

    std::string password = FileReader::getPassword(host.passPath);

    if (!password.empty()) {
        SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)password.c_str());
        SSL_CTX_set_default_passwd_cb(ctx, [](char* buf, int size, int, void* userdata) -> int {
           const auto* pwd = static_cast<const char*>(userdata);
            strncpy(buf, pwd, size);
            buf[size - 1] = '\0';
            return static_cast<int>(strlen(buf));
        });
    }



    if (SSL_CTX_use_PrivateKey_file(ctx, host.keyPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("Failed to load private key");
    }
}