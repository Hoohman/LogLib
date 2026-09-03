#include "Filter.h"
#include "LogRecord.h"
#include <set>
#include <string>
#include <utility>

LevelFilter::LevelFilter(LogLevel minLevel) : logLevel_(minLevel) {}

bool LevelFilter::filter(const LogRecord& record) const {
    return record.level >= logLevel_;
}

NameFilter::NameFilter(std::set<std::string> allowed)
    : allowed_(std::move(allowed)) {}

bool NameFilter::filter(const LogRecord& record) const {
    if (allowed_.empty()) {
        return true;
    }
    if (allowed_.contains(record.loggerName)) {
        return true;
    }
    return false;
}
