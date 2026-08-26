#include "Logger.h"
#include <gtest/gtest.h>
#include <memory>
#include <set>
#include <string>
#include <utility>

class StringSink : public ISink {
public:
    std::string lastMessage;
    int writeCount = 0;

    void flush() override {}

    void write(const LogRecord& record) override {
        lastMessage = formatter_->format(record);
        writeCount++;
    }

protected:
    bool open() override { return true; }
    void close() override {}
    void onError(const std::string&) override {}
};

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto sink = std::make_unique<StringSink>();
        testSink = sink.get();

        logger = std::make_unique<Logger>("TestLogger");
        logger->addSink(std::move(sink));
    }

    StringSink* testSink;
    std::unique_ptr<Logger> logger;
};

TEST_F(LoggerTest, BasicLoggingWorks) {
    logger->info("Hello world");

    EXPECT_EQ(testSink->writeCount, 1);
    EXPECT_NE(testSink->lastMessage.find("Hello world"), std::string::npos);
    EXPECT_NE(testSink->lastMessage.find("INFO"), std::string::npos);
}

TEST_F(LoggerTest, LevelFilterDropsLowerLevels) {
    logger->addFilter(std::make_unique<LevelFilter>(LogLevel::WARN));

    logger->info("info msg");
    EXPECT_EQ(testSink->writeCount, 0);

    logger->warn("warn msg");
    EXPECT_EQ(testSink->writeCount, 1);

    logger->error("error msg");
    EXPECT_EQ(testSink->writeCount, 2);
}

TEST_F(LoggerTest, NameFilterWorks) {
    logger->addFilter(
        std::make_unique<NameFilter>(std::set<std::string>{"AllowedLogger"}));

    logger->info("msg");
    EXPECT_EQ(testSink->writeCount, 0);
}

TEST_F(LoggerTest, TimestampEnricherAddsTimestamp) {
    logger->addEnricher(std::make_unique<TimestampEnricher>());

    logger->info("msg");
    EXPECT_EQ(testSink->writeCount, 1);

    EXPECT_NE(testSink->lastMessage.find("2026"), std::string::npos);
}

TEST_F(LoggerTest, JsonFormatterWorks) {
    testSink->setFormatter(std::make_unique<JsonFormatter>());

    logger->info("test message", {{"key", "value"}});

    EXPECT_NE(testSink->lastMessage.find("\"level\":\"INFO\""),
              std::string::npos);
    EXPECT_NE(testSink->lastMessage.find("\"msg\":\"test message\""),
              std::string::npos);
    EXPECT_NE(testSink->lastMessage.find("\"key\":\"value\""),
              std::string::npos);
}

TEST(LogManagerTest, BuilderConfiguresLoggerCorrectly) {
    auto sink = std::make_unique<StringSink>();
    auto sinkPtr = sink.get();

    Logger& log = LogManager::instance()
                      .buildLogger("BuilderTestLogger")
                      .withLevelFilter(LogLevel::ERROR)
                      .addCustomSink(std::move(sink))
                      .build();

    log.warn("warning");
    EXPECT_EQ(sinkPtr->writeCount, 0);

    log.fatal("fatal error");
    EXPECT_EQ(sinkPtr->writeCount, 1);
}
