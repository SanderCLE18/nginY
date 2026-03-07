//
// Created by Sande on 02.03.2026.
//
#pragma once
#include "Connection.h"
#include <openssl/ssl.h>

class HttpsConnection : Connection {
private:
    SSL* ssl;

public:
    HttpsConnection(int socket, SSL_CTX* ctx) {
        this->fd = socket;
        this->ssl = SSL_new(ctx);
        SSL_set_fd(ssl, fd);
    }

    ssize_t read(void *buf, size_t len) override {
        return SSL_read(ssl, buf, len);
    }
    ssize_t write(const void* buf, size_t len) override {
        return SSL_write(ssl, buf, len);
    }
};

