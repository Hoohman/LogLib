#include "Formatter.h"
#include "LogRecord.h"
#include <format>
#include <string>

std::string IFormatter::logLevel_to_string(const LogLevel& level) {
    switch (level) {
    case LogLevel::TRACE:
        return "TRACE";
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::FATAL:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}

std::string PlainTextFormatter::format(const LogRecord& record) const {
    std::string result = std::format("[{}] [{:<5}] [{}] {}", record.timestamp,
                                     logLevel_to_string(record.level),
                                     record.loggerName, record.message);

    for (const auto& [key, value] : record.fields) {
        result += std::format(" {}={}", key, value);
    }
    return result;
}

std::string JsonFormatter::escape_json(const std::string& s) const {
    std::string escaped;
    escaped.reserve(s.size());

    for (char c : s) {
        switch (c) {
        case '"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            if (c >= 0 && c < 32) {
                escaped += ' ';
            } else {
                escaped += c;
            }
            break;
        }
    }
    return escaped;
}

std::string JsonFormatter::format(const LogRecord& record) const {
    std::string result = "{\n";

    result += std::format(
        "\t\"level\":\"{}\",\n\t\"logger\":\"{}\",\n\t\"msg\":\"{}\"",
        logLevel_to_string(record.level), escape_json(record.loggerName),
        escape_json(record.message));

    for (const auto& [key, value] : record.fields) {
        result += ",\n";
        result += std::format("\t\"{}\":\"{}\"", escape_json(key),
                              escape_json(value));
    }

    result += "\n}";
    return result;
}
