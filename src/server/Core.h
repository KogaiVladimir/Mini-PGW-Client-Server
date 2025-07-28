//Core.h

#ifndef CORE_H
#define CORE_H

#include <memory>
#include <thread>
#include <atomic>
#include <csignal>
#include <nlohmann/json.hpp>
#include "SessionManager.h"
#include "UdpServer.h"
#include "HttpServer.h"
#include "../ConfigDirPath.h"

class Core {

public:

    explicit Core(std::shared_ptr<Logger> log);
    
    ~Core();

    // Запуск всех серверов (UDP + HTTP)
    void start();

    // Корректная остановка серверов
    void stop();

    // Проверка состояния работы
    bool isRunning() const;

private:
    // Загрузка конфигурации из JSON файла
    void loadConfig(const std::string& config_path);

    // Инициализация менеджера сессий
    void initSessionManager();

    // Инициализация UDP сервера
    void initUdpServer();

    // Инициализация HTTP сервера
    void initHttpServer();

    std::shared_ptr<Logger> m_log;                      // Логгер системы
    std::atomic<bool> m_shutdown_flag;                  // Флаг завершения работы
    nlohmann::json m_config;                            // Конфигурация системы

    std::shared_ptr<SessionManager> m_session_manager;  // Менеджер сессий
    std::unique_ptr<UdpServer> m_udp_server;            // UDP сервер
    std::unique_ptr<HttpServer> m_http_server;          // HTTP сервер
};

#endif // CORE_H