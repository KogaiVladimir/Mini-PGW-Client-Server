//UdpServer.h

#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <arpa/inet.h>
#include <memory>
#include <string>
#include <system_error>
#include <netinet/in.h>
#include <unistd.h>
#include "SessionManager.h"

class UdpServer {

public:

    UdpServer(const uint16_t& port, 
              std::shared_ptr<ISessionManager> session_manager,
              std::shared_ptr<Logger> log);

    ~UdpServer();

    // Основной метод запуска сервера
    void start();

    void stop();

    int getSockfd() const; 

private:
    // Создание UDP сокета
    int createSocket();

    // Привязка сокета к порту
    void bindSocket(const int& sockfd);

    // Прием и обработка сообщений
    void receiveAndProcess(const int& sockfd);

    // Безопасное закрытие сокета
    void closeSocket(const int& sockfd) noexcept;

    std::thread m_udp_server_thread;                   // Поток UDP сервера

    uint16_t m_port;                                   // Порт сервера

    std::shared_ptr<ISessionManager> m_session_manager;// Менеджер сессий

    std::shared_ptr<Logger> m_log;                     // Логгер

    int m_sockfd;

    std::mutex m_queue_mutex;

    std::atomic<bool> m_running;

};

#endif // UDPSERVER_H