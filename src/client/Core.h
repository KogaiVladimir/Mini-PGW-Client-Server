//Core.h

#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include "../Logger.h"
#include "../ConfigDirPath.h"
#include "UdpClient.h"

using json = nlohmann::json;

class Core {

public:

    explicit Core(std::shared_ptr<Logger> log);

    void sendRequest(const std::string& imsi);
    
private:

    std::shared_ptr<Logger> m_log;
    
    nlohmann::json m_config;

    std::unique_ptr<UdpClient> m_udp_client;

};