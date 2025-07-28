//UdpServer.cpp

#include "UdpServer.h"

UdpServer::UdpServer(const uint16_t& port,
    std::shared_ptr<ISessionManager> session_manager,
    std::shared_ptr<Logger> log) 
: m_port(port), m_session_manager(session_manager), m_log(log),
 m_sockfd(-1), m_running(false) {}

UdpServer::~UdpServer() {
    stop();
}

// Главный метод запуска сервера
void UdpServer::start() {
    if (m_running) return;
    
    m_running = true;
    try {
        createSocket();
        bindSocket(getSockfd());
        m_udp_server_thread = std::thread([this]() {
            receiveAndProcess(getSockfd());
        });
    } catch (const std::exception& e) {
        if (m_running) {
            m_log->sendToLog("[FATAL] " + std::string(e.what()));
        }
    }
}

void UdpServer::stop() {
    if (!m_running) return;
    
    m_running = false;
    int wakeup_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (wakeup_sock >= 0) {
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(m_port);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
        
        sendto(wakeup_sock, "", 0, 0, 
              (sockaddr*)&server_addr, sizeof(server_addr));
        close(wakeup_sock);
    }
    
    if (m_udp_server_thread.joinable()) {
        m_udp_server_thread.join();
    }
    
    closeSocket(m_sockfd);
    m_sockfd = -1;
}

int UdpServer::getSockfd() const {
    return m_sockfd;
}

int UdpServer::createSocket() {
    if (m_sockfd >= 0) closeSocket(m_sockfd);
    m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    std::cout << "UdpServer::m_sockfd: " << std::to_string(m_sockfd) << std::endl;
    if (m_sockfd < 0) {
        m_log->sendToLog("Failed to create UDP socket");
        throw std::system_error(
            errno, 
            std::generic_category(), 
            "Failed to create socket"
        );
    }
    m_log->sendToLog("Create UDP socket");
    return m_sockfd;
}

void UdpServer::bindSocket(const int& sockfd) {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(m_port);

    if (bind(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        closeSocket(sockfd);
        m_log->sendToLog("Failed to bind UDP socket");
        throw std::system_error(
            errno,
            std::generic_category(),
            "Failed to bind socket"
        );
    }
    m_log->sendToLog("Bind UDP socket");
}

void UdpServer::receiveAndProcess(const int& sockfd) {
    char buffer[1024];
    sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    while (m_running) {
        // std::unique_lock<std::mutex> lock(m_queue_mutex);
        if (!m_running) break;
        try {
            // Принимаем сообщение
            ssize_t status_receive = recvfrom(
                sockfd, buffer, sizeof(buffer), 0,
                (sockaddr*)&client_addr, &len
            );
            if (status_receive < 0) {
                m_log->sendToLog("Failed to receive data");
                throw std::runtime_error("Failed to receive data");
            }

            // Обрабатываем IMSI
            std::string imsi(buffer, status_receive);
            m_log->sendToLog("IMSI from UE: " + imsi);
            std::string response = m_session_manager->handleImsi(imsi);

            // Отправляем ответ
            ssize_t status_sendto = sendto(
                sockfd, response.c_str(), response.size(), 0,
                (sockaddr*)&client_addr, len
            );
            // spdlog::debug("UdpServer::status_sendto: {}", status_sendto);
            if (status_sendto < 0) {
                m_log->sendToLog("Failed to send data");
                throw std::runtime_error("Failed to send response");
            }
            m_log->sendToLog("Send to UE: " + imsi + ", " + response);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] " << e.what() << std::endl;
            // Можно добавить логирование через spdlog
        }
    }
}

// Закрытие сокета (безопасное)
void UdpServer::closeSocket(const int& sockfd) noexcept {
    if (sockfd >= 0) close(sockfd);
}