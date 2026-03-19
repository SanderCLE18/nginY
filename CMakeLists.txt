cmake_minimum_required(VERSION 3.20)

project(nginY LANGUAGES CXX)

# C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
        src/core/main.cpp
        src/core/WebServer.cpp
        src/core/WebServer.h
        src/utils/StaticResourceManager.h
        src/utils/StaticResourceManager.cpp
        src/network/ProxyConnection.cpp
        src/network/ProxyConnection.h
        src/core/ThreadPool.h
        src/utils/ServerConfig.cpp
        src/utils/ServerConfig.h
        src/utils/LinkedBlockingQueue.h
        src/utils/Logger.cpp
        src/utils/Logger.h
        src/network/connections/Connection.h
        src/network/connections/HttpsConnection.h
        src/network/connections/HttpConnection.h
        src/network/SSLContext.cpp
        src/network/SSLContext.h
        src/utils/FileReader.cpp
        src/utils/FileReader.h
)

# Executable
add_executable(nginY ${SOURCES})

# OpenSSL
find_package(OpenSSL REQUIRED)
target_link_libraries(nginY PRIVATE OpenSSL::Crypto OpenSSL::SSL)


# Linux / GCC / Clang warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")

    target_compile_options(nginY PRIVATE -stdlib=libc++)

    target_link_options(nginY PRIVATE -stdlib=libc++)

endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(nginY PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()