//HttpServer.h

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <memory>
#include <string>
#include <cstdint>
#include <httplib.h>
#include "SessionManager.h"

class HttpServer {

public:

    HttpServer(const uint16_t& port,
               std::shared_ptr<ISessionManager> session_manager,
               std::shared_ptr<Logger> log);

    ~HttpServer();

    // Запуск сервера
    void start();

    // Остановка сервера
    void stop();

private:
    // Настройка маршрутов сервера
    void setupRoutes();
    
    std::atomic<bool> m_running;

    std::thread m_http_server_thread; 

    const uint16_t m_port;                              // Порт сервера
    std::shared_ptr<ISessionManager> m_session_manager; // Менеджер сессий
    std::shared_ptr<Logger> m_log;                      // Логгер
    std::unique_ptr<httplib::Server> m_server;          // Экземпляр сервера httplib
};

#endif // HTTPSERVER_H