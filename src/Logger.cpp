#include "Logger.h"
#include "Enricher.h"
#include "Filter.h"
#include "LogRecord.h"
#include "Sink.h"
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

Logger::Logger(std::string name) : name_(std::move(name)) {
    enrichers_ = std::make_unique<BaseEnricher>();
}

void Logger::trace(std::string_view msg,
                   const std::map<std::string, std::string>& fields) {
    log(LogLevel::TRACE, msg, fields);
}

void Logger::debug(std::string_view msg,
                   const std::map<std::string, std::string>& fields) {
    log(LogLevel::DEBUG, msg, fields);
}

void Logger::info(std::string_view msg,
                  const std::map<std::string, std::string>& fields) {
    log(LogLevel::INFO, msg, fields);
}

void Logger::warn(std::string_view msg,
                  const std::map<std::string, std::string>& fields) {
    log(LogLevel::WARN, msg, fields);
}

void Logger::error(std::string_view msg,
                   const std::map<std::string, std::string>& fields) {
    log(LogLevel::ERROR, msg, fields);
}

void Logger::fatal(std::string_view msg,
                   const std::map<std::string, std::string>& fields) {
    log(LogLevel::FATAL, msg, fields);
}

void Logger::addFilter(std::unique_ptr<IFilter> filter) {
    filters_.push_back(std::move(filter));
}

void Logger::addEnricher(std::unique_ptr<IEnricher> enricher) {
    enricher->setNext(std::move(enrichers_));
    enrichers_ = std::move(enricher);
}

void Logger::addSink(std::unique_ptr<ISink> sink) {
    sinks_.push_back(std::move(sink));
}

void Logger::log(LogLevel level, std::string_view message,
                 const std::map<std::string, std::string>& fields) {
    LogRecord record{.level = level,
                     .message = std::string(message),
                     .loggerName = name_,
                     .fields = fields};

    for (const auto& filter : filters_) {
        if (!filter->filter(record)) {
            return;
        }
    }

    enrichers_->enrich(record);

    for (const auto& sink : sinks_) {
        sink->write(record);
    }
}

LoggerBuilder::LoggerBuilder(Logger& logger) : logger_(logger) {}

LoggerBuilder& LoggerBuilder::withFilter(std::unique_ptr<IFilter> filter) {
    logger_.addFilter(std::move(filter));
    return *this;
}

LoggerBuilder&
LoggerBuilder::withEnricher(std::unique_ptr<IEnricher> enricher) {
    logger_.addEnricher(std::move(enricher));
    return *this;
}

LoggerBuilder& LoggerBuilder::withSink(std::unique_ptr<ISink> sink) {
    logger_.addSink(std::move(sink));
    return *this;
}

Logger& LoggerBuilder::build() { return logger_; }
