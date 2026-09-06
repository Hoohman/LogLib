#pragma once

#include "LogRecord.h"
#include <set>
#include <string>

class IFilter {
public:
    virtual ~IFilter() = default;
    virtual bool filter(const LogRecord&) const = 0;
};

class LevelFilter : public IFilter {
public:
    LevelFilter(LogLevel minLevel);
    bool filter(const LogRecord& record) const override;

private:
    LogLevel logLevel_;
};

class NameFilter : public IFilter {
public:
    NameFilter(std::set<std::string> allowed);
    bool filter(const LogRecord& record) const override;

private:
    std::set<std::string> allowed_;
};
