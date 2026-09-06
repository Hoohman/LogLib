#pragma once

#include "LogRecord.h"
#include <memory>

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
    void enrich(LogRecord& record) const override;
    void doEnrich(LogRecord& record) const override;
};

class TimestampEnricher : public IEnricher {
public:
    void doEnrich(LogRecord& record) const override;
};
