//LoggerTest.cpp

#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <vector>
#include <regex>
#include "../src/Logger.h"

TEST(LoggerTest, CreatesLogFile) {
    const std::string path = "test_create.log";
    {
        Logger logger(path);
        std::ifstream log_file(path);
        EXPECT_TRUE(log_file.good());
    }
    std::remove(path.c_str());
}

TEST(LoggerTest, WritesSingleMessage) {
    const std::string path = "test_single.log";
    const std::string msg = "Test message 1";
    
    Logger logger(path);
    logger.start();
    logger.sendToLog(msg);
    logger.flush();
    logger.stop();
    
    std::ifstream log_file(path);
    std::string content((std::istreambuf_iterator<char>(log_file)), 
                       std::istreambuf_iterator<char>());
    EXPECT_NE(content.find(msg), std::string::npos);
    
    std::remove(path.c_str());
}

TEST(LoggerTest, HandlesConcurrentWrites) {
    const std::string path = "test_concurrent.log";
    constexpr int kThreads = 5;
    constexpr int kMessages = 20;
    
    Logger logger(path);
    logger.start();
    std::vector<std::thread> threads;
    
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&logger, i]() {
            for (int j = 0; j < kMessages; ++j) {
                logger.sendToLog(std::to_string(i) + "_" + std::to_string(j));
            }
        });
    }
    
    for (auto& t : threads) t.join();
    logger.flush();
    logger.stop();
    
    std::ifstream log_file(path);
    int lines = std::count(std::istreambuf_iterator<char>(log_file),
                         std::istreambuf_iterator<char>(), '\n');
    EXPECT_EQ(lines, kThreads * kMessages);
    
    std::remove(path.c_str());
}

TEST(LoggerTest, AddsTimestamps) {
    const std::string path = "test_timestamps.log";
    Logger logger(path);
    logger.start();
    logger.sendToLog("Timestamp test");
    logger.flush();
    logger.stop();
    
    std::ifstream log_file(path);
    std::string line;
    std::getline(log_file, line);

    EXPECT_TRUE(std::regex_search(line, 
        std::regex(R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] Timestamp test)")));
    
    std::remove(path.c_str());
}

TEST(LoggerTest, StopPreventsWrites) {
    const std::string path = "test_stop.log";
    Logger logger(path);
    logger.start();
    logger.stop();
    
    logger.sendToLog("Should not appear");
    std::ifstream log_file(path);
    EXPECT_EQ(log_file.peek(), std::ifstream::traits_type::eof());
    
    std::remove(path.c_str());
}

TEST(LoggerTest, FlushWritesImmediately) {
    const std::string path = "test_flush.log";
    Logger logger(path);
    logger.start();
    
    logger.sendToLog("Before flush");
    logger.flush();
    logger.stop();
    logger.sendToLog("After flush");
    
    std::ifstream log_file(path);
    std::string content((std::istreambuf_iterator<char>(log_file)),
                       std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("Before flush"), std::string::npos);
    
    std::remove(path.c_str());
}

TEST(LoggerTest, HandlesLargeMessages) {
    const std::string path = "test_large.log";
    const std::string big_msg(1'000'000, 'x');
    
    Logger logger(path);
    logger.start();
    logger.sendToLog(big_msg);
    logger.flush();
    logger.stop();
    
    std::ifstream log_file(path);
    std::string content((std::istreambuf_iterator<char>(log_file)),
                       std::istreambuf_iterator<char>());
    EXPECT_GE(content.size(), big_msg.size());
    
    std::remove(path.c_str());
}