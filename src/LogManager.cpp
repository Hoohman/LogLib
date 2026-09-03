#include "LogManager.h"
#include "Enricher.h"
#include "Filter.h"
#include "Formatter.h"
#include "LogRecord.h"
#include "Logger.h"
#include "Sink.h"
#include <memory>
#include <string>
#include <utility>

LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
}

Logger& LogManager::getLogger(const std::string& name) {
    auto it = loggers_.find(name);
    if (it != loggers_.end()) {
        return *it->second;
    }

    auto sink = std::make_unique<ConsoleSink>();
    sink->setFormatter(std::make_unique<PlainTextFormatter>());
    return buildLogger(name)
        .withFilter(std::make_unique<LevelFilter>(LogLevel::INFO))
        .withEnricher(std::make_unique<TimestampEnricher>())
        .withSink(std::move(sink))
        .build();
}

LoggerBuilder LogManager::buildLogger(const std::string& name) {
    auto [it, inserted] =
        loggers_.try_emplace(name, std::make_unique<Logger>(name));
    return LoggerBuilder(*it->second);
}
