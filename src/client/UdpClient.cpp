//UdpClient.cpp

#include "UdpClient.h"

UdpClient::UdpClient(const std::string& server_ip, 
    const uint16_t& server_port,
    std::shared_ptr<Logger> log)
    : m_log(log) {
    createSocket();
    setupServerAddress(server_ip, server_port);
}

UdpClient::~UdpClient() {
    if (m_sockfd >= 0) {
        close(m_sockfd);
        spdlog::info("Socket closed");
    }
}

void UdpClient::createSocket() {
    m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sockfd < 0) {
        throw std::system_error(errno, std::generic_category(), "socket() failed");
    }
    spdlog::info("Socket created (fd: {})", m_sockfd);
}

void UdpClient::setupServerAddress(const std::string& ip, uint16_t port) {
    m_server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { .s_addr = inet_addr(ip.c_str()) }
    };

    if (m_server_addr.sin_addr.s_addr == INADDR_NONE) {
        throw std::invalid_argument("Invalid IP address: " + ip);
    }
    m_log->sendToLog("Socket address configured: " + ip + ":" + std::to_string(port));
    spdlog::info("Server address configured: {}:{}", ip, port);
}

std::string UdpClient::sendRequest(const std::string& imsi) {
    socklen_t len = sizeof(m_server_addr);
    ssize_t send_to = sendto(m_sockfd, imsi.c_str(), imsi.size(), 0,
          (sockaddr*)&m_server_addr, len);
    char buffer[1024];
    ssize_t n = recvfrom(m_sockfd, buffer, sizeof(buffer), 0,
                    (sockaddr*)&m_server_addr, &len);    
    if (n < 0) {
        m_log->sendToLog("No response from server");
        spdlog::warn("No response from server");
        return "error";
    }

    m_log->sendToLog("Server response: " + std::string(buffer, n));
    spdlog::info("Server response: {}", std::string(buffer, n));
    return std::string(buffer, n);
}