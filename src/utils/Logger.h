#pragma once
#include "LinkedBlockingQueue.h"
#include <chrono>
#include <format>

class Logger {
private:
    static Logger * LOGGER;
    static LinkedBlockingQueue<std::string> queue;
    Logger() {
        while (true) {

        }
    }

public:

    static void log(const std::string &warning, int errorCode) {

        auto now = std::chrono::system_clock::now();
        const std::string t_c = std::format("{:%Y-%m-%d %H:%M:%S}", now);

        std::string log = "[" + t_c + "]" + warning + std::to_string(errorCode) + '\n';

        queue.offer(log);
    }
};
