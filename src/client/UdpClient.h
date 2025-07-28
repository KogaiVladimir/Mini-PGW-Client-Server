//UdpClient.h

#pragma once

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/socket.h>
#include <cstring>
#include "../Logger.h"

class UdpClient {

public:

    UdpClient(const std::string& server_ip, 
        const uint16_t& server_port,
        std::shared_ptr<Logger> log);

    ~UdpClient();

    std::string sendRequest(const std::string& imsi);

private:

    void createSocket();
    
    void setupServerAddress(const std::string& ip, uint16_t port);

    int m_sockfd;

    sockaddr_in m_server_addr;

    std::shared_ptr<Logger> m_log;

};