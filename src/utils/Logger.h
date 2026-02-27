#pragma once
#include "LinkedBlockingQueue.h"
#include <chrono>
#include <format>
#include <thread>
#include <fstream>
#include <optional>

class Logger {
private:
    static std::jthread logThread;
    static Logger errorLogger;
    static LinkedBlockingQueue<std::string> queue;
    static std::stop_token stopToken;
    Logger() {
        logThread = std::jthread([]() {

            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
            std::string t_c = ss.str();

            std::ofstream outFile(t_c+"_Log.txt", std::ios::app);

            while (true) {
                auto item = queue.getListItem();

                //Shutdown called inside the queue.
                if (!item.has_value()) {
                    return;
                }

                outFile << item.value() << std::endl;
            }

            outFile.close();
        });
    }

public:
    static Logger & getLogger() {
        static Logger logger;
        return logger;
    }

    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void requestStop();
    static void log(const std::string &warning, int errorCode);
};
