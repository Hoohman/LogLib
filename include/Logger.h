#pragma once

#include "Enricher.h"
#include "Filter.h"
#include "LogRecord.h"
#include "Sink.h"
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class Logger {
public:
    explicit Logger(std::string name);

    void trace(std::string_view msg,
               const std::map<std::string, std::string>& fields = {});
    void debug(std::string_view msg,
               const std::map<std::string, std::string>& fields = {});
    void info(std::string_view msg,
              const std::map<std::string, std::string>& fields = {});
    void warn(std::string_view msg,
              const std::map<std::string, std::string>& fields = {});
    void error(std::string_view msg,
               const std::map<std::string, std::string>& fields = {});
    void fatal(std::string_view msg,
               const std::map<std::string, std::string>& fields = {});

    void addFilter(std::unique_ptr<IFilter> filter);
    void addEnricher(std::unique_ptr<IEnricher> enricher);
    void addSink(std::unique_ptr<ISink> sink);

private:
    void log(LogLevel level, std::string_view message,
             const std::map<std::string, std::string>& fields);

    std::string name_;
    std::vector<std::unique_ptr<IFilter>> filters_;
    std::unique_ptr<IEnricher> enrichers_;
    std::vector<std::unique_ptr<ISink>> sinks_;
};

class LoggerBuilder {
public:
    explicit LoggerBuilder(Logger& logger);

    LoggerBuilder& withFilter(std::unique_ptr<IFilter> filter);
    LoggerBuilder& withEnricher(std::unique_ptr<IEnricher> enricher);
    LoggerBuilder& withSink(std::unique_ptr<ISink> sink);

    Logger& build();

private:
    Logger& logger_;
};
