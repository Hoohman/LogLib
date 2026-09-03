#pragma once

#include "Formatter.h"
#include "LogRecord.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

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
    void flush() override;
    void write(const LogRecord& record) override;

protected:
    bool open() override;
    void close() override;
    void onError(const std::string& err) override;
};

class FileSink : public ISink {
public:
    FileSink(std::filesystem::path path, size_t maxSizeByte);
    ~FileSink() override;
    void flush() override;
    void write(const LogRecord& record) override;

protected:
    bool open() override;
    void close() override;
    void onError(const std::string& err) override;

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
    void flush() override;
    void write(const LogRecord& record) override;

protected:
    bool open() override;
    void close() override;
    void onError(const std::string& err) override;

private:
    std::unique_ptr<ISink> downstream_;
    size_t batchSize_;
    std::vector<LogRecord> buffer_;
};
