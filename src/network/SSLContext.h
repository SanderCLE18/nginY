//
// Created by Sande on 08.03.2026.
//

#pragma once
#include <stdexcept>
#include <openssl/types.h>
#include <string>
#include <openssl/ssl.h>
#include "../utils/ServerConfig.h"

/**
 * SSLContext class for managing SSL/TLS context.
 */
class SSLContext {
private:
    /**
     * Pointer to the stored SSL context.
     */
    SSL_CTX *ctx;

public:
    /**
     * @brief Constructor for SSLContext.
     * Initializes SSL context with provided server configuration.
     *
     * @param config reference to the server configuration
     */
    SSLContext(const ServerConfig::Config &config);

    /**
     * @brief Destructor for SSLContext.
     * Frees the SSL context resources.
     */
    ~SSLContext() {
        SSL_CTX_free(ctx);
    }

    /**
     * @brief Copy constructor deleted to prevent copying SSL context.
     */
    SSLContext(const SSLContext &) = delete;


    /**
     * @brief Copy assignment operator is deleted.
     * Prevents copying SSL contexts to ensure unique ownership.
     */
    SSLContext &operator=(const SSLContext &) = delete;

    /**
    * @brief Get the underlying SSL context pointer.
    * @note The return value should not be discarded as it provides the primary
    * interface to the underlying OpenSSL structure.
    *
    * @return Pointer to the SSL context.
    */
    [[nodiscard]] SSL_CTX *get() const { return ctx; }
};
