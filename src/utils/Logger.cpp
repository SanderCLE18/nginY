//
// Created by Sande on 27.02.2026.
//
#include "Logger.h"
#include <string>
#include <chrono>
#include <format>


std::jthread Logger::logThread;
LinkedBlockingQueue<std::string> Logger::queue;
std::stop_token Logger::stopToken;

void Logger::log(const std::string &warning, int errorCode) {

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");

    std::string log = "[" + ss.str() + "]" + warning + std::to_string(errorCode);

    getLogger().queue.offer(log);
}

void Logger::requestStop() {
    logThread.request_stop();
}

Logger::~Logger() {
    queue.cleanup();
}
