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
    /**
     * @brief Pointer to the SSL structure.
     */
    SSL* ssl;

public:

    /**
     * @brief Constructor for the HTTPS connections.
     * * The constructor initializes the SSL connection with the provided socket and SSL context.
     * * This does also handle the redirection to HTTPS if necessary.
     *
     * @param socket The socket to be used for the connection
     * @param ctx Pointer to the SSL context
     * @throw std::runtime_error if the connection fails
     */
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
     * @brief Tells the client to move to the active HTTPS page
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

    /**
     * @brief Reads from a connection using the openSSL library
     *
     * @param buf Pointer to the buffer to read from
     * @param len Length of the buffer
     * @return Number of bytes read
     */
    ssize_t read(void *buf, size_t len) override {
        return SSL_read(ssl, buf, len);
    }

    /**
     * @brief Writes data to a connection using the openSSL library
     *
     * @param buf Pointer to the buffer to be written in
     * @param len Number of bytes to write
     * @return Actual number of written bytes
     */
    ssize_t write(const void* buf, size_t len) override {
        return SSL_write(ssl, buf, len);
    }

    /**
     * @brief Peeks into the received message and checks if it is HTTP
     *
     * @param socket
     * @return True if the first byte is not TLS, otherwise false.
     */
    bool isHttp(int socket) {
        char probe;
        int n = recv(socket, &probe, 1, MSG_PEEK);

        return probe != 0x16;
    }

    /**
     * @brief closes the connection.
     */
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

    /**
     * @brief shuts down the connection.
     *
     * @param how unused value
     */
    void shutdown(std::optional<int> how) override {
        int placeholder = how.value();
        int ret = SSL_shutdown(ssl);

        if (ret == 0) {
            SSL_shutdown(ssl);
        }
    }

    /**
    * @brief Sets the connection to either blocking or non-blocking mode
    *
    * @param blocking If true, connections will block
    */
    void setBlocking(bool blocking) const {
        unsigned long mode = blocking ? 0 : 1;
        ioctl(fd, FIONBIO, &mode);
    }

protected:

    /**
     * @brief File descriptor for the connection
     */
    int fd;
};