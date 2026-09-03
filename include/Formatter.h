#pragma once

#include "LogRecord.h"
#include <string>

class IFormatter {
public:
    virtual ~IFormatter() = default;
    virtual std::string format(const LogRecord&) const = 0;
    static std::string logLevel_to_string(const LogLevel& level);
};

class PlainTextFormatter : public IFormatter {
public:
    std::string format(const LogRecord& record) const override;
};

class JsonFormatter : public IFormatter {
public:
    std::string format(const LogRecord& record) const override;

private:
    std::string escape_json(const std::string& s) const;
};
