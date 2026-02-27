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
        src/FileHandler.cpp
        src/FileHandler.h
        src/network/ProxyHandler.cpp
        src/network/ProxyHandler.h
        src/ThreadPool.h
        src/utilIO/ProxyConfig.cpp
        src/utilIO/ProxyConfig.h
        src/utils/LinkedBlockingQueue.h
        src/utils/Logger.h
        src/utils/Logger.cpp
)

# Executable
add_executable(nginY ${SOURCES})

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