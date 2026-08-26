#include "Logger.h"
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
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

void IEnricher::enrich(LogRecord& record) const {
    next_->enrich(record);
    doEnrich(record);
}

void IEnricher::setNext(std::unique_ptr<IEnricher> next) {
    next_ = std::move(next);
}

void BaseEnricher::enrich(LogRecord& record) const {}

void BaseEnricher::doEnrich(LogRecord& record) const {}

void TimestampEnricher::doEnrich(LogRecord& record) const {
    auto now = std::chrono::floor<std::chrono::milliseconds>(
        std::chrono::system_clock::now());
    record.timestamp = now;
}

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

void ISink::setFormatter(std::unique_ptr<IFormatter> fmt) {
    formatter_ = std::move(fmt);
}

ISink::ISink() { formatter_ = std::make_unique<PlainTextFormatter>(); }

ConsoleSink::ConsoleSink() { open(); }

ConsoleSink::~ConsoleSink() { close(); }

bool ConsoleSink::open() { return true; }

void ConsoleSink::close() {}

void ConsoleSink::flush() { std::cout.flush(); }

void ConsoleSink::onError(const std::string& err) { std::cerr << err << '\n'; }

void ConsoleSink::write(const LogRecord& record) {
    auto formattedMessage = formatter_->format(record);
    std::cout << formattedMessage << '\n';
}

FileSink::FileSink(std::filesystem::path path, size_t maxSizeByte)
    : path_(std::move(path)), maxSizeByte_(maxSizeByte) {
    open();
}

FileSink::~FileSink() { close(); }

bool FileSink::open() {
    file_.open(path_, std::ios_base::app);
    if (!file_.is_open()) {
        onError("File (" + path_.string() + ") cant be opened\n");
        return false;
    }
    return true;
}

void FileSink::close() { file_.close(); }

void FileSink::flush() { file_.flush(); }

void FileSink::onError(const std::string& err) { std::cerr << err << '\n'; }

void FileSink::write(const LogRecord& record) {
    auto formattedMessage = formatter_->format(record);
    file_ << formattedMessage << '\n';
    flush();
    bytesWritten_ = std::filesystem::file_size(path_);
    if (bytesWritten_ >= maxSizeByte_) {
        rotate();
    }
}

void FileSink::rotate() {
    file_.close();
    const int maxBackups = 3;

    for (size_t i = maxBackups; i > 0; --i) {
        std::filesystem::path old_path =
            path_.string() + (i == 1 ? "" : "." + std::to_string(i - 1));
        std::filesystem::path new_path =
            path_.string() + "." + std::to_string(i);

        if (std::filesystem::exists(old_path)) {
            if (std::filesystem::exists(new_path)) {
                std::filesystem::remove(new_path);
            }
            std::filesystem::rename(old_path, new_path);
        }
    }

    file_.open(path_);
    bytesWritten_ = 0;
}

BufferedSink::BufferedSink(std::unique_ptr<ISink> downstream, size_t batchSize)
    : downstream_(std::move(downstream)), batchSize_(batchSize) {}

BufferedSink::~BufferedSink() { close(); }

bool BufferedSink::open() { return true; }

void BufferedSink::close() { flush(); }

void BufferedSink::flush() {
    for (const auto& msg : buffer_) {
        downstream_->write(msg);
    }
    downstream_->flush();
    buffer_.clear();
}

void BufferedSink::onError(const std::string& err) {}

void BufferedSink::write(const LogRecord& record) {
    buffer_.push_back(record);
    if (buffer_.size() >= batchSize_) {
        flush();
    }
}

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

LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
}

Logger& LogManager::getLogger(const std::string& name) {
    auto it = loggers_.find(name);
    if (it != loggers_.end()) {
        return *it->second;
    }
    return buildLogger(name)
        .withLevelFilter(LogLevel::INFO)
        .withTimestamp()
        .addConsoleSink(std::make_unique<PlainTextFormatter>())
        .build();
}

LoggerBuilder LogManager::buildLogger(const std::string& name) {
    auto [it, inserted] = loggers_.try_emplace(name, nullptr);
    if (inserted) {
        it->second = std::make_unique<Logger>(name);
    }
    return LoggerBuilder(*it->second);
}

LoggerBuilder::LoggerBuilder(Logger& logger) : logger_(logger) {}

LoggerBuilder& LoggerBuilder::withLevelFilter(LogLevel minLevel) {
    logger_.addFilter(std::make_unique<LevelFilter>(minLevel));
    return *this;
}

LoggerBuilder& LoggerBuilder::withNameFilter(std::set<std::string> allowed) {
    logger_.addFilter(std::make_unique<NameFilter>(std::move(allowed)));
    return *this;
}

LoggerBuilder& LoggerBuilder::withTimestamp() {
    logger_.addEnricher(std::make_unique<TimestampEnricher>());
    return *this;
}

LoggerBuilder&
LoggerBuilder::addConsoleSink(std::unique_ptr<IFormatter> formatter) {
    auto sink = std::make_unique<ConsoleSink>();
    sink->setFormatter(std::move(formatter));
    logger_.addSink(std::move(sink));
    return *this;
}

LoggerBuilder&
LoggerBuilder::addFileSink(const std::string& path, size_t maxSizeByte,
                           std::unique_ptr<IFormatter> formatter) {
    auto sink = std::make_unique<FileSink>(path, maxSizeByte);
    sink->setFormatter(std::move(formatter));
    logger_.addSink(std::move(sink));
    return *this;
}

LoggerBuilder&
LoggerBuilder::addBufferedFileSink(const std::string& path,
                                   size_t maxFileSizeByte, size_t batchSize,
                                   std::unique_ptr<IFormatter> formatter) {
    auto fileSink = std::make_unique<FileSink>(path, maxFileSizeByte);
    fileSink->setFormatter(std::move(formatter));
    auto sink = std::make_unique<BufferedSink>(std::move(fileSink), batchSize);
    logger_.addSink(std::move(sink));
    return *this;
}

LoggerBuilder& LoggerBuilder::addCustomSink(std::unique_ptr<ISink> sink) {
    logger_.addSink(std::move(sink));
    return *this;
}

Logger& LoggerBuilder::build() { return logger_; }
