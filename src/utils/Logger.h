#pragma once
#include "LinkedBlockingQueue.h"

class Logger {
private:
    static Logger * LOGGER;
    static LinkedBlockingQueue<String> queue;
    Logger() {
        Thread thread = new t
    }
public:

    template<typename Args ...args>
    static void log(const std::string &warning, Args&& args) {

    }
};
