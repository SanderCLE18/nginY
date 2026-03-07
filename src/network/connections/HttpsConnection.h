//
// Created by Sande on 02.03.2026.
//
#pragma once
#include <unistd.h>

#include "Connection.h"
#include <openssl/ssl.h>

class HttpsConnection : public Connection {
private:
    SSL* ssl;

public:
    HttpsConnection(int socket, SSL_CTX* ctx) {
        this->fd = socket;
        this->ssl = SSL_new(ctx);
        setBlocking(true);
        SSL_set_fd(ssl, fd);
    }

    ssize_t read(void *buf, size_t len) override {
        return SSL_read(ssl, buf, len);
    }
    ssize_t write(const void* buf, size_t len) override {
        return SSL_write(ssl, buf, len);
    }

    void close() override {
        if (this->ssl) {

            SSL_shutdown(this->ssl);
            SSL_free(this->ssl);
            this->ssl = nullptr;
        }

        if (this->fd != -1) {
            ::close(this->fd);
            this->fd = -1;
        }
    }
    void shutdown(int how) override {
        int ret = SSL_shutdown(ssl);

        if (ret == 0) {
            SSL_shutdown(ssl);
        }
    }
};

