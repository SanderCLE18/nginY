//
// Created by Sande on 02.03.2026.
//
#pragma once
#include <unistd.h>

#include "Connection.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
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

        if (isHttp(socket)) {
            httpRedirect(socket);
            ::close(socket);
            throw std::runtime_error("Redirecting to https");
        }

        int result = SSL_accept(ssl);

        if (result <= 0) {
            int error = SSL_get_error(ssl, result);
            std::string errorStack;
            unsigned long err;
            char buf[256];
            while ((err = ERR_get_error()) != 0) {
                ERR_error_string_n(err, buf, sizeof(buf));
                errorStack += "\n  -> ";
                errorStack += buf;
            }

            Logger::log("SSL_accept failed | result: " + std::to_string(result)
                      + " | ssl_error: " + std::to_string(error)
                      + " | stack: " + errorStack, -1);

            throw std::runtime_error("TLS handshake failed");
        }
    }

    /**
     * Tells the client to move to the active page
     *
     * @param socket incoming socket
     */
    void httpRedirect(int socket) {
        char buffer[4096];
        recv(socket, buffer, sizeof(buffer) - 1, 0);

        std::string path = "/";
        std::istringstream ss(buffer);
        std::string method, fullPath;
        if (ss >> path >> fullPath) {
            path = fullPath;
        }
        std::string header = "HTTP/1.1 301 Moved Permanently\r\n"
        "Location: https://localhost:8443" + path + "\r\n"
        "Connection: close\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

        send(socket, header.data(), header.size(), 0);
    }
    ssize_t read(void *buf, size_t len) override {
        return SSL_read(ssl, buf, len);
    }
    ssize_t write(const void* buf, size_t len) override {
        return SSL_write(ssl, buf, len);
    }

    bool isHttp(int socket) {
        char probe;
        int n = recv(socket, &probe, 1, MSG_PEEK);
        toupper(probe);
        if (n > 0 && (probe == 'H' || probe == 'P' || probe == 'G' || probe == 'D')) {
            return true;
        }
        return false;
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