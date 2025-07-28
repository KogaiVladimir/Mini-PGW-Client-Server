//Core.cpp

#include "Core.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

Core::Core(std::shared_ptr<Logger> log) 
    : m_log(log) 
{
    try {
        // Загрузка конфигурации
        std::ifstream config_file(configDirPath::clientConfig());
        if (!config_file.is_open()) {
            throw std::runtime_error("Failed to open config file");
        }
        config_file >> m_config;
        
        // Инициализация UDP клиента
        std::string server_ip = m_config["server_ip"];
        uint16_t server_port = m_config["server_port"];
        
        m_udp_client = std::make_unique<UdpClient>(
            server_ip, 
            server_port, 
            m_log
        );
        
        spdlog::info("Core initialized successfully");
    } 
    catch (const std::exception& e) {
        m_log->sendToLog("Core initialization failed: " + std::string(e.what()));
        throw;
    }
}

void Core::sendRequest(const std::string& imsi) 
{
    try {
        if (!m_udp_client) {
            throw std::runtime_error("UDP client not initialized");
        }
        
        m_log->sendToLog("Sending request for IMSI: " + imsi);
        std::string response = m_udp_client->sendRequest(imsi);
        m_log->sendToLog("Received response: " + response);
    } 
    catch (const std::exception& e) {
        m_log->sendToLog("Request failed: " + std::string(e.what()));
        throw;
    }
}