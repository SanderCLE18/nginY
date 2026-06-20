//
// Created by Sande on 20.06.2026.
//

#include "SNIContext.h"

SNIContext::SNIContext(const ServerConfig::Config &config) {
    for (const auto &item : config.content) {
        auto ctx = std::make_unique<SSLContext>(item);
        SSL_CTX_set_tlsext_servername_callback(ctx->get(), sniCallback);
        SSL_CTX_set_tlsext_servername_arg(ctx->get(), this);
        contexts[item.hostName] = std::move(ctx);
    }
}

SSL_CTX *SNIContext::getDefault() const {
    if (contexts.empty()) {
        return nullptr;
    }

    return contexts.begin()->second->get();
}

int SNIContext::sniCallback(SSL* ssl, int*, void* arg) {
    auto* self = static_cast<SNIContext*>(arg);
    const char* name = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    if (!name) {
        return SSL_TLSEXT_ERR_NOACK;
    }
    auto it = self->contexts.find(name);
    if (it == self->contexts.end()) {
        return SSL_TLSEXT_ERR_NOACK;
    }

    SSL_set_SSL_CTX(ssl, it->second->get());
    return SSL_TLSEXT_ERR_OK;
}
