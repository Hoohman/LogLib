#include "Enricher.h"
#include "LogRecord.h"
#include <chrono>
#include <memory>
#include <utility>

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
