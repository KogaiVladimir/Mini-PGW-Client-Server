//SessionManagerTest.cpp

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "../src/server/SessionManager.h"
#include "../src/Logger.h"

namespace fs = std::filesystem;

// Вспомогательная функция для создания временного файла
std::string create_temp_file(const std::string& prefix) {
    auto path = fs::temp_directory_path() / (prefix + std::to_string(std::time(nullptr)));
    return path.string();
}

TEST(SessionManagerBasicTest, CreatesAndManagesSessions) {
    const auto cdr_path = create_temp_file("cdr_");
    const auto log_path = create_temp_file("log_");
    auto logger = std::make_shared<Logger>(log_path);
    logger->start();

    const std::vector<std::string> blacklist = {"123456789012345"};
    SessionManager manager(5, 100, cdr_path, blacklist, logger);
    manager.startCleanupTimer();
    
    // Проверка создания сессии
    EXPECT_EQ(manager.handleImsi("001010123456789"), "created");
    EXPECT_TRUE(manager.isSessionActive("001010123456789"));
    
    // Проверка повторного использования
    EXPECT_EQ(manager.handleImsi("001010123456789"), "exists");
    
    // Проверка черного списка
    EXPECT_EQ(manager.handleImsi("123456789012345"), "rejected");
    
    logger->stop();
    fs::remove(cdr_path);
    fs::remove(log_path);
}

TEST(SessionManagerBasicTest, HandlesInvalidImsi) {
    const auto cdr_path = create_temp_file("cdr_");
    const auto log_path = create_temp_file("log_");
    auto logger = std::make_shared<Logger>(log_path);
    logger->start();

    SessionManager manager(5, 100, cdr_path, {}, logger);
    manager.startCleanupTimer();
    
    EXPECT_EQ(manager.handleImsi("invalid_imsi_with_letters"), "rejected");
    EXPECT_EQ(manager.handleImsi("12345678901234567890"), "rejected"); // слишком длинный
    
    logger->stop();
    fs::remove(cdr_path);
    fs::remove(log_path);
}

TEST(SessionManagerBasicTest, CleanupExpiredSessions) {
    const auto cdr_path = create_temp_file("cdr_");
    const auto log_path = create_temp_file("log_");
    auto logger = std::make_shared<Logger>(log_path);
    logger->start();

    SessionManager manager(1, 100, cdr_path, {}, logger); 
    manager.startCleanupTimer();
    
    manager.handleImsi("001010123456789");
    EXPECT_TRUE(manager.isSessionActive("001010123456789"));
    
    for (int i = 0; i < 30; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!manager.isSessionActive("001010123456789")) break;
    }
    EXPECT_FALSE(manager.isSessionActive("001010123456789"));
    
    manager.stopCleanupTimer();
    logger->stop();
    fs::remove(cdr_path);
    fs::remove(log_path);
}

TEST(SessionManagerBasicTest, WritesToCdrFile) {
    const auto cdr_path = create_temp_file("cdr_");
    const auto log_path = create_temp_file("log_");
    auto logger = std::make_shared<Logger>(log_path);
    logger->start();

    const std::vector<std::string> blacklist = {"001010123456789"};
    SessionManager manager(5, 100, cdr_path, blacklist, logger);
    manager.startCleanupTimer();
    
    manager.handleImsi("001010123456780");
    manager.handleImsi("001010123456789");
    
    logger->flush();
    
    std::ifstream cdr_file(cdr_path);
    std::string content((std::istreambuf_iterator<char>(cdr_file)), 
                 std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("001010123456780, created") != std::string::npos);
    EXPECT_TRUE(content.find("001010123456789, rejected_blacklist") != std::string::npos);
    
    logger->stop();
    fs::remove(cdr_path);
    fs::remove(log_path);
}

TEST(SessionManagerBasicTest, GracefulShutdown) {
    const auto cdr_path = create_temp_file("cdr_");
    const auto log_path = create_temp_file("log_");
    auto logger = std::make_shared<Logger>(log_path);
    logger->start();

    SessionManager manager(60, 10, cdr_path, {}, logger);
    manager.startCleanupTimer();
    
    manager.handleImsi("001010123456780");
    manager.handleImsi("001010123456781");
    
    manager.gracefulShutdown();
    
    EXPECT_FALSE(manager.isSessionActive("001010123456780"));
    EXPECT_FALSE(manager.isSessionActive("001010123456781"));
    
    logger->stop();
    fs::remove(cdr_path);
    fs::remove(log_path);
}

TEST(SessionManagerConcurrencyTest, HandlesConcurrentAccess) {
    const auto cdr_path = create_temp_file("cdr_");
    const auto log_path = create_temp_file("log_");
    auto logger = std::make_shared<Logger>(log_path);
    logger->start();

    SessionManager manager(60, 100, cdr_path, {}, logger);
    manager.startCleanupTimer();
    
    constexpr int kThreads = 5;
    constexpr int kIterations = 50;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&manager, i]() {
            for (int j = 0; j < kIterations; ++j) {
                std::string imsi = std::to_string(i) + std::to_string(j);
                manager.handleImsi(imsi);
                manager.isSessionActive(imsi);
            }
        });
    }
    
    for (auto& t : threads) t.join();
    manager.stopCleanupTimer();
    
    // Проверяем что все сессии обработаны
    std::ifstream cdr_file(cdr_path);
    int lines = std::count(std::istreambuf_iterator<char>(cdr_file),
                         std::istreambuf_iterator<char>(), '\n');
    EXPECT_GE(lines, kThreads * kIterations);
    
    logger->stop();
    fs::remove(cdr_path);
    fs::remove(log_path);
}