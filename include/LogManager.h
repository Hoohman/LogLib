#pragma once

#include "Logger.h"
#include <memory>
#include <string>
#include <unordered_map>

class LogManager {
public:
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    static LogManager& instance();

    Logger& getLogger(const std::string& name);
    LoggerBuilder buildLogger(const std::string& name);

private:
    LogManager() = default;
    std::unordered_map<std::string, std::unique_ptr<Logger>> loggers_;
};
