#pragma once
#include "LinkedBlockingQueue.h"
#include <chrono>
#include <format>
#include <thread>
#include <fstream>
#include <optional>

/**
 * @brief Logger Singleton class for logging messages to a file
 */
class Logger {
private:
    /**
     * @brief logthread
     */
    static std::jthread logThread;

    /**
     * @brief Logger object
     */
    static Logger errorLogger;

    /**
     * @brief LinkedBlockingQueue that stores log messages
     */
    static LinkedBlockingQueue<std::string> queue;

    /**
     * @brief Stop token for signaling to the logger when it's time to stop.
     */
    static std::stop_token stopToken;

    /**
     * @brief Logger constructor, starts a thread to process log messages
     */
    Logger() {
        std::atexit([]() {
            queue.cleanup();
        });
        logThread = std::jthread([]() {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
            std::string t_c = ss.str();

            std::ofstream outFile(t_c + "_Log.txt", std::ios::app);

            while (!stopToken.stop_requested()) {
                auto item = queue.getListItem();
                if (!item.has_value()) {
                    return;
                }

                outFile << item.value() << std::endl;
            }
            while (true) {
                auto item = queue.tryGetListItem(); // non-blocking drain
                if (!item.has_value()) break;
                outFile << item.value() << "\n";
            }

            outFile.close();
        });
    }

public:
    /**
     * @brief Static method for returning a reference to the logger instance
     *
     * @return Returns a reference to the logger instance
     */
    static Logger &getLogger() {
        static Logger logger;
        return logger;
    }

    /**
     * @brief Destructor for Logger class
     */
    ~Logger();

    /**
     * @brief Disallows the Logger class from being copied
     */
    Logger(const Logger &) = delete;

    /**
     * @brief Copy assignment operator for Logger class
     * Disallows copying.
     */
    Logger &operator=(const Logger &) = delete;

    /**
     * @brief Function for requesting stop of Logger singleton.
     */
    static void requestStop();

    /**
     * @brief Logs a warning message with an associated error code.
     *
     * @param warning Warning message to be logged
     * @param errorCode Error code to be logged
     */
    static void log(const std::string &warning, int errorCode);
};
