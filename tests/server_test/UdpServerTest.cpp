// //UdpServerTest.cpp

#include <iostream>
#include <gtest/gtest.h>
#include <thread>
#include <future>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../src/server/UdpServer.h"
#include "../src/server/SessionManager.h"
#include "../src/Logger.h"

namespace {
    const uint16_t TEST_PORT = 54321;
    const std::string TEST_IMSI = "123456789012345";
}

const uint16_t m_udp_port = 8080;

// Вспомогательная функция для отправки тестового сообщения
void send_udp_message(const std::string& message, uint16_t port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    auto sent_to = sendto(sock, message.c_str(), message.size(), 0,
        (sockaddr*)&server_addr, sizeof(server_addr));

    close(sock);
}

TEST(UdpServerTest, CreatesAndBindsSocket) {
    auto logger = std::make_shared<Logger>("test_udp.log");
    auto session_mgr = std::make_shared<SessionManager>
        (60, 100, "test_cdr.csv", std::vector<std::string>{}, logger);

    UdpServer server(m_udp_port, session_mgr, logger);
    // Проверяем сокет до start()
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(server.getSockfd(), -1); // Должен быть -1 до запуска

    server.start();
    
    // Даем больше времени на инициализацию
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_GE(server.getSockfd(), 0);
    server.stop();
    
    // Проверяем что сокет закрыт после stop()
    EXPECT_EQ(server.getSockfd(), -1);
}

TEST(UdpServerTest, HandlesIncomingMessages) {
    auto logger = std::make_shared<Logger>("test_udp.log");
    auto session_mgr = std::make_shared<SessionManager>(60, 100, "test_cdr.csv", std::vector<std::string>{}, logger);
    
    UdpServer server(TEST_PORT, session_mgr, logger);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Отправляем тестовое сообщение
    send_udp_message(TEST_IMSI, TEST_PORT);
    
    // Даем время на обработку
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Проверяем что сессия создана
    EXPECT_TRUE(session_mgr->isSessionActive(TEST_IMSI));
    
    server.stop();
}

TEST(UdpServerTest, StopsGracefully) {
    auto logger = std::make_shared<Logger>("test_udp.log");
    auto session_mgr = std::make_shared<SessionManager>(60, 100, "test_cdr.csv", std::vector<std::string>{}, logger);
    
    UdpServer server(TEST_PORT, session_mgr, logger);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(server.getSockfd() >= 0);
    
    server.stop();
    
    // Проверяем что сокет закрыт
    EXPECT_EQ(server.getSockfd(), -1);
}

TEST(UdpServerTest, HandlesMultipleMessages) {
    auto logger = std::make_shared<Logger>("test_udp.log");
    auto session_mgr = std::make_shared<SessionManager>(60, 100, "test_cdr.csv", std::vector<std::string>{}, logger);
    
    UdpServer server(TEST_PORT, session_mgr, logger);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    for (int i = 0; i < 5; ++i) {
        const std::string imsi = TEST_IMSI.substr(0, 14) + std::to_string(i);
        
        // Отправляем сообщение
        send_udp_message(imsi, TEST_PORT);
        
        // Ждём обработки с таймаутом
        bool session_active = false;
        for (int attempt = 0; attempt < 10; ++attempt) {
            if (session_mgr->isSessionActive(imsi)) {
                session_active = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        EXPECT_TRUE(session_active) << "Session not created for IMSI: " << imsi;
    }
    
    server.stop();
}

TEST(UdpServerTest, HandlesInvalidMessages) {
    auto logger = std::make_shared<Logger>("test_udp.log");
    auto session_mgr = std::make_shared<SessionManager>(60, 100, "test_cdr.csv", std::vector<std::string>{}, logger);
    
    UdpServer server(TEST_PORT, session_mgr, logger);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Отправляем невалидное сообщение
    send_udp_message("invalid_imsi", TEST_PORT);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Проверяем что сессия не создана
    EXPECT_FALSE(session_mgr->isSessionActive("invalid_imsi"));
    
    server.stop();
}

TEST(UdpServerTest, HandlesBlacklistedImsi) {
    const std::vector<std::string> blacklist = {"123456789000000"};
    auto logger = std::make_shared<Logger>("test_udp.log");
    auto session_mgr = std::make_shared<SessionManager>(60, 100, "test_cdr.csv", blacklist, logger);
    
    UdpServer server(TEST_PORT, session_mgr, logger);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Отправляем IMSI из черного списка
    send_udp_message("123456789000000", TEST_PORT);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Проверяем что сессия не создана
    EXPECT_FALSE(session_mgr->isSessionActive("123456789000000"));
    
    server.stop();
}