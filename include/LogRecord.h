#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <string>

enum class LogLevel : uint8_t { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

struct LogRecord {
    LogLevel level;
    std::string message;
    std::string loggerName;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> fields;
};
