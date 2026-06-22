//
// Created by Sande on 25.02.2026.
//

#pragma once
#include <string>
#include <vector>

/**
 * @brief Server configuration class, parses configuration files and provides access to proxy rules and server settings.
 */
class ServerConfig {
    /**
     * @brief Method for parsing and getting the IP-address and port from a line in the configuration
     *
     * @param value reference to a line in the configuration file.
     * @return tuple with IP and port number for the proxy
     */
    static std::tuple<std::string, std::string> parseProxy(const std::string &value);

public:
    /**
     * @brief Structure that saves the different proxy rules
     */
    struct ProxyRules {
        std::string location; ///< The URI path
        std::string host;     ///< Destination host
        std::string port;     ///< Destination port
        bool proxy;           ///< Whether rule is a proxy or not.
    };

    /**
     * @brief Structure for managing different servers. Each gets its own rule vector and name.
     *
     */
    struct VirtualHost {
        std::vector<ProxyRules> content; ///< The host's rules.
        std::vector<int> httpsPort;      ///< The host's stored https ports
        std::vector<int> httpPort;       ///< The host's stored http ports
        std::string hostName;            ///< The host's name
        std::string passPath;            ///< Path to the password file
        std::string certPath;            ///< Path to the certificate file
        std::string keyPath;             ///< Path to the key file
    };

    /**
     * @brief Structure that saves the different server settings
     */
    struct Config {
        std::vector<VirtualHost> content; ///< Vector containing all the different proxy rules
        bool found;                       ///< Value signaling if the configuration file was found
    };

    /**
     * @brief Method for parsing the configuration file
     *
     * @param path Reference path to the configuration file
     * @return Returns a Config structure.
     */
    static Config parseConfig(const std::string &path);

private:
    static VirtualHost parseVirtualHost(std::ifstream &file);

    static void parseListenPorts(std::string& line, VirtualHost &host);
};
