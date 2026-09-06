#include "Sink.h"
#include "Formatter.h"
#include "LogRecord.h"
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

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
    try {
        auto formattedMessage = formatter_->format(record);
        std::cout << formattedMessage << '\n';
    } catch (const std::exception& e) {
        onError(std::string("ConsoleSink write failed: ") + e.what());
    }
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
    try {
        auto formattedMessage = formatter_->format(record);
        file_ << formattedMessage << '\n';
        flush();
        bytesWritten_ = std::filesystem::file_size(path_);
        if (bytesWritten_ >= maxSizeByte_) {
            rotate();
        }
    } catch (const std::exception& e) {
        onError(std::string("FileSink write failed: ") + e.what());
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
    try {
        for (const auto& msg : buffer_) {
            downstream_->write(msg);
        }
        downstream_->flush();
    } catch (const std::exception& e) {
        onError(std::string("BufferedSink flush failed: ") + e.what());
    }

    buffer_.clear();
}

void BufferedSink::onError(const std::string& err) { std::cerr << err << '\n'; }

void BufferedSink::write(const LogRecord& record) {
    buffer_.push_back(record);
    if (buffer_.size() >= batchSize_) {
        flush();
    }
}
