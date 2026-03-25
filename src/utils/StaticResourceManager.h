//
// Created by Sande on 09.02.2026.
//
#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

/**
 * @brief Class serving static resources
 */
class StaticResourceManager {
private:
    /**
     * @brief Method for determining the file type based on the file extension
     *
     * @param path Reference to the path string
     * @return Returns the file type as a string
     */
    static std::string getFileType(const std::string &path);

public:
    /**
     * @brief Helper structure for representing HTTP responses
     */
    struct Response {
        std::string header;        ///< The HTTP response header
        std::vector<char> content; ///< The HTTP response content
        bool found;                ///< Indicates whether the resource was found
    };

    /**
     * @brief Constructor for StaticResourceManager
     */
    StaticResourceManager();

    /**
     * @brief Destructor for StaticResourceManager
     */
    ~StaticResourceManager();

    /**
     * @brief Method for serving static resources
     *
     * @param path path to the requested site
     * @return Response object containing the HTTP response
     */
    static Response getSite(const std::string &path);

    /**
     * @brief Method for transforming a URL path to a system path
     *
     * @param path path to the requested site
     * @return a system path to the requested site
     */
    static std::string getUrlPath(std::string &path);

    /**
     * @brief Method for determining the length of the content-length from a header in bytes
     *
     * @param header header
     * @return the length for the content in bytes
     */
    static long long getContentLength(const std::string &header);
};
