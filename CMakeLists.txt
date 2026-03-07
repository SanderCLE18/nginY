cmake_minimum_required(VERSION 3.20)

project(nginY LANGUAGES CXX)

# C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
        src/main.cpp
        src/network/WebServer.cpp
        src/network/WebServer.h
        src/FileHandler.h
        src/FileHandler.cpp
        src/network/ProxyConnection.cpp
        src/network/ProxyConnection.h
        src/ThreadPool.h
        src/utilIO/ProxyConfig.cpp
        src/utilIO/ProxyConfig.h
        src/utils/LinkedBlockingQueue.h
        src/utils/Logger.cpp
        src/utils/Logger.h
        src/network/connections/Connection.h
        src/network/connections/HttpsConnection.h
        src/network/connections/HttpConnection.h

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