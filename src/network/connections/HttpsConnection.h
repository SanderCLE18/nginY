//
// Created by Sande on 02.03.2026.
//
#pragma once
#include <unistd.h>

#include "Connection.h"
#include <openssl/ssl.h>
#include "../../utils/Logger.h"

class HttpsConnection : public Connection {
private:
    SSL* ssl;

public:
    HttpsConnection(int socket, SSL_CTX* ctx) {
        this->fd = socket;
        this->ssl = SSL_new(ctx);
        setBlocking(true);
        SSL_set_fd(ssl, fd);

        int result = SSL_accept(ssl);

        if (result <= 0) {
            int error = SSL_get_error(ssl, result);
            Logger::log("SSL_accept failed with error: ", error);
            SSL_free(ssl);
            ::close(fd);
            throw std::runtime_error("TLS handshake failed");
        }
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
    void shutdown(std::optional<int> how) override {
        int placeholder = how.value();
        int ret = SSL_shutdown(ssl);

        if (ret == 0) {
            SSL_shutdown(ssl);
        }
    }
};

