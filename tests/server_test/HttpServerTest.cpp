//HttpServerTest.cpp

#include <iostream>
#include <gtest/gtest.h>
#include <httplib.h>
#include <thread>
#include <memory>
#include "../src/server/HttpServer.h"
#include "../src/server/SessionManager.h"
#include "../src/Logger.h"

namespace {
    constexpr uint16_t TEST_PORT = 8080;
    const std::string TEST_IMSI = "12345678901234";
}

uint16_t get_random_port() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(8000, 9000);
    return dist(gen);
}

// Вспомогательная функция для проверки доступности порта
bool is_port_available(uint16_t port) {
    httplib::Server svr;
    bool result = svr.bind_to_port("0.0.0.0", port);
    svr.stop();
    return result;
}

TEST(HttpServerBasicTest, StartsAndStops) {
    auto logger = std::make_shared<Logger>("test_http.log");
    auto session_mgr = std::make_shared<SessionManager>(60, 100, "test_cdr.csv", std::vector<std::string>{}, logger);
    
    const uint16_t port = get_random_port();
    std::cout << "HttpServerRequestTest, StartsAndStops:Port "
        << port << std::endl;
    HttpServer server(port, session_mgr, logger);
    EXPECT_NO_THROW(server.start());
    
    // Даем время на запуск
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_NO_THROW(server.stop());
}

TEST(HttpServerRequestTest, HandlesValidSubscriberCheck) {
    if (!is_port_available(TEST_PORT)) {
        GTEST_SKIP() << "Port " << TEST_PORT << " is not available";
        std::cout << "Port " << TEST_PORT << " is not available" << std::endl;
    }

    auto logger = std::make_shared<Logger>("test_http.log");
    auto session_mgr = std::make_shared<SessionManager>(60, 100, "test_cdr.csv", std::vector<std::string>{}, logger);
    
    // Создаем тестовую сессию
    std::string imsi = TEST_IMSI + "0"; // Делаем 15 символов
    session_mgr->handleImsi(imsi);
    
    const uint16_t port = get_random_port();
    HttpServer server(port, session_mgr, logger);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Отправляем запрос
    httplib::Client client("localhost", port);
    auto res = client.Get(("/check_subscriber?imsi=" + imsi).c_str());
    
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "active");
    server.stop();
}

TEST(HttpServerRequestTest, ReturnsErrorForMissingImsi) {
    if (!is_port_available(TEST_PORT)) {
        GTEST_SKIP() << "Port " << TEST_PORT << " is not available";
    }

    auto logger = std::make_shared<Logger>("test_http.log");
    auto session_mgr = std::make_shared<SessionManager>(60, 100, "test_cdr.csv", std::vector<std::string>{}, logger);
    
    const uint16_t port = get_random_port();
    HttpServer server(port, session_mgr, logger);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    httplib::Client client("localhost", port);
    auto res = client.Get("/check_subscriber");
    
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_EQ(res->body, "IMSI parameter is missing");
    
    server.stop();
}

TEST(HttpServerStressTest, HandlesMultipleSubscribers) {
    if (!is_port_available(TEST_PORT)) {
        GTEST_SKIP() << "Port " << TEST_PORT << " is not available";
    }

    auto logger = std::make_shared<Logger>("test_http.log");
    auto session_mgr = std::make_shared<SessionManager>(60, 100, "test_cdr.csv", std::vector<std::string>{}, logger);
    
    // Создаем тестовые сессии
    for (int i = 0; i < 5; ++i) {
        std::string imsi = TEST_IMSI + std::to_string(i); // 15 символов
        session_mgr->handleImsi(imsi);
    }
    
    const uint16_t port = get_random_port();
    HttpServer server(port, session_mgr, logger);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    httplib::Client client("localhost", port);
    
    // Проверяем все созданные сессии
    for (int i = 0; i < 5; ++i) {
        std::string imsi = TEST_IMSI + std::to_string(i);
        auto res = client.Get(("/check_subscriber?imsi=" + imsi).c_str());
        
        ASSERT_TRUE(res) << "Failed request for IMSI: " << imsi;
        EXPECT_EQ(res->status, 200) << "Wrong status for IMSI: " << imsi;
        EXPECT_EQ(res->body, "active") << "Wrong body for IMSI: " << imsi;
    }
    
    server.stop();
}

TEST(HttpServerCommandTest, HandlesStopCommand) {
    if (!is_port_available(TEST_PORT)) {
        GTEST_SKIP() << "Port " << TEST_PORT << " is not available";
    }

    auto logger = std::make_shared<Logger>("test_http.log");
    auto session_mgr = std::make_shared<SessionManager>(60, 100, "test_cdr.csv", std::vector<std::string>{}, logger);
    
    const uint16_t port = get_random_port();
    HttpServer server(port, session_mgr, logger);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    httplib::Client client("localhost", port);
    client.set_connection_timeout(10);
    client.set_read_timeout(10);
    auto res = client.Get("/stop");
    
    ASSERT_TRUE(res);
    EXPECT_EQ(res->body, "Server is shutting down...");
    
    // Даем время на остановку
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}