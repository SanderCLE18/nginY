cmake_minimum_required(VERSION 3.20)

project(nginY LANGUAGES CXX)

# C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
        src/main.cpp
        src/WebServer.cpp
        src/FileHandler.cpp
        src/FileHandler.h
        src/ProxyHandler.cpp
        src/ProxyHandler.h
        src/configEnum.cpp
        src/ThreadPool.cpp
        src/ThreadPool.h
)

# Executable
add_executable(nginY ${SOURCES})

# Linux / GCC / Clang warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(nginY PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()