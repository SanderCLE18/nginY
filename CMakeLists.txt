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

#gTest
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(nginY_tests
        tests/core/test_Threadpool.cpp
        tests/network/connections/test_HttpConnection.cpp
        tests/network/connections/gTestConnection.cpp
        tests/network/connections/gTestConnectionParser.h
)
target_include_directories(nginY_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(nginY_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock_main
        OpenSSL::Crypto
        OpenSSL::SSL
)
include(GoogleTest)
gtest_discover_tests(nginY_tests)

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