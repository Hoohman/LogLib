#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class LogLevel : uint8_t { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

struct LogRecord {
    LogLevel level;
    std::string message;
    std::string loggerName;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> fields;
};

class IFilter {
public:
    virtual ~IFilter() = default;
    virtual bool filter(const LogRecord&) const = 0;
};

class LevelFilter : public IFilter {
public:
    LevelFilter(LogLevel minLevel);
    virtual bool filter(const LogRecord& record) const override;

private:
    LogLevel logLevel_;
};

class NameFilter : public IFilter {
public:
    NameFilter(std::set<std::string> allowed);
    virtual bool filter(const LogRecord& record) const override;

private:
    std::set<std::string> allowed_;
};

class IEnricher {
public:
    virtual ~IEnricher() = default;
    virtual void enrich(LogRecord& record) const;
    void setNext(std::unique_ptr<IEnricher> next);

protected:
    virtual void doEnrich(LogRecord&) const = 0;

private:
    std::unique_ptr<IEnricher> next_;
};

class BaseEnricher : public IEnricher {
public:
    virtual void enrich(LogRecord& record) const override;
    virtual void doEnrich(LogRecord& record) const override;
};

class TimestampEnricher : public IEnricher {
public:
    virtual void doEnrich(LogRecord& record) const override;
};

class IFormatter {
public:
    virtual ~IFormatter() = default;
    virtual std::string format(const LogRecord&) const = 0;
    static std::string logLevel_to_string(const LogLevel&);
};

class PlainTextFormatter : public IFormatter {
public:
    virtual std::string format(const LogRecord& record) const override;
};

class JsonFormatter : public IFormatter {
public:
    virtual std::string format(const LogRecord& record) const override;

private:
    std::string escape_json(const std::string& s) const;
};

class ISink {
public:
    virtual ~ISink() = default;
    virtual void flush() = 0;
    virtual void write(const LogRecord& record) = 0;

    void setFormatter(std::unique_ptr<IFormatter> fmt);

protected:
    ISink();
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual void onError(const std::string& err) = 0;
    std::unique_ptr<IFormatter> formatter_;
};

class ConsoleSink : public ISink {
public:
    ConsoleSink();
    ~ConsoleSink() override;
    virtual void flush() override;
    virtual void write(const LogRecord& record) override;

protected:
    virtual bool open() override;
    virtual void close() override;
    virtual void onError(const std::string& err) override;
};

class FileSink : public ISink {
public:
    FileSink(std::filesystem::path path, size_t maxSizeByte);
    ~FileSink() override;
    virtual void flush() override;
    virtual void write(const LogRecord& record) override;

protected:
    virtual bool open() override;
    virtual void close() override;
    virtual void onError(const std::string& err) override;

private:
    void rotate();

    std::filesystem::path path_;
    size_t maxSizeByte_;
    std::ofstream file_;
    size_t bytesWritten_ = 0;
};

class BufferedSink : public ISink {
public:
    BufferedSink(std::unique_ptr<ISink> downstream, size_t batchSize);
    ~BufferedSink() override;
    virtual void flush() override;
    virtual void write(const LogRecord& record) override;

protected:
    virtual bool open() override;
    virtual void close() override;
    virtual void onError(const std::string& err) override;

private:
    std::unique_ptr<ISink> downstream_;
    size_t batchSize_;
    std::vector<LogRecord> buffer_;
};

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

    LoggerBuilder& withLevelFilter(LogLevel minLevel);
    LoggerBuilder& withNameFilter(std::set<std::string> allowed);
    LoggerBuilder& withTimestamp();
    LoggerBuilder& addConsoleSink(std::unique_ptr<IFormatter> formatter);
    LoggerBuilder& addFileSink(const std::string& path, size_t maxSizeByte,
                               std::unique_ptr<IFormatter> formatter);
    LoggerBuilder& addBufferedFileSink(const std::string& path,
                                       size_t maxSizeByte, size_t batchSize,
                                       std::unique_ptr<IFormatter> formatter);
    LoggerBuilder& addCustomSink(std::unique_ptr<ISink> sink);

    Logger& build();

private:
    Logger& logger_;
};

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
