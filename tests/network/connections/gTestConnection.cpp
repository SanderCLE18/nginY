//
// Created by Sande on 21.03.2026.
//

#include <gmock/gmock.h>
#include "network/connections/Connection.h"

class gTestConnection : public Connection {
public:
    MOCK_METHOD(ssize_t, read, (void* buffer, size_t size), (override));
    MOCK_METHOD(ssize_t, write, (const void* buffer, size_t size), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(void, shutdown, (std::optional<int> how), (override));
};