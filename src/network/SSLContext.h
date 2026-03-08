//
// Created by Sande on 08.03.2026.
//

#pragma once
#include <stdexcept>
#include <openssl/types.h>
#include <string>
#include <openssl/ssl.h>
#include "../utils/ServerConfig.h"

class SSLContext {
private:
    SSL_CTX* ctx;
public:
    SSLContext(const ServerConfig::Config& config);

    ~SSLContext() {
        SSL_CTX_free(ctx);
    }

    SSLContext(const SSLContext&) = delete;
    SSLContext& operator=(const SSLContext&) = delete;

    [[nodiscard]] SSL_CTX* get() const { return ctx; }
};

