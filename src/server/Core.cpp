//Core.cpp

#include "Core.h"

namespace {
    Core* core_instance = nullptr;

    void signal_handler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            spdlog::warn("Received shutdown signal");
            if (core_instance) {
                core_instance->stop();
            }
        }
    }
}

Core::Core(std::shared_ptr<Logger> log)
: m_log(log), m_shutdown_flag(false) {
    spdlog::info("Initializing core...");
    try {
        loadConfig(configDirPath::serverConfig());
        initSessionManager();
        initUdpServer();
        initHttpServer();
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        core_instance = this;
    } catch (const std::exception& e) {
        spdlog::critical("Initialization failed: {}", e.what());
        throw;
    }
}

Core::~Core() {
    if (core_instance == this) core_instance = nullptr;
    stop();
    spdlog::info("Core cleanup completed");
}

void Core::start() {
    spdlog::info("Starting servers...");

    m_session_manager->startCleanupTimer();
    m_udp_server->start();
    m_http_server->start();

    spdlog::info("Server started successfully");
    spdlog::info("UDP port: {}", m_config["udp_port"].get<uint16_t>());
    spdlog::info("HTTP port: {}", m_config["http_port"].get<uint16_t>());
}

void Core::stop() {
    if (m_shutdown_flag) return;
    
    m_shutdown_flag = true;
    spdlog::info("Shutting down servers...");
    
    m_udp_server->stop();
    m_http_server->stop();
    m_session_manager->stopCleanupTimer();
}

bool Core::isRunning() const { 
    return !m_shutdown_flag; 
}

void Core::loadConfig(const std::string& config_path) {
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + config_path);
    }
    m_config = nlohmann::json::parse(config_file);
    spdlog::debug("Config loaded successfully");
}

void Core::initSessionManager() {
    try {
        spdlog::debug("Initializing SessionManager...");
        m_session_manager = std::make_shared<SessionManager>(
            m_config["session_timeout_sec"].get<unsigned int>(),
            m_config["graceful_shutdown_rate"].get<unsigned int>(),
            m_config["cdr_file"].get<std::string>(),
            m_config["blacklist"].get<std::vector<std::string>>(),
            m_log
        );
        spdlog::info("SessionManager initialized successfully");
    } catch (const std::exception& e) {
        spdlog::error("SessionManager initialization failed: {}", e.what());
        throw std::runtime_error("Cannot initialize SessionManager: " + std::string(e.what()));
    }
}

void Core::initUdpServer() {
    try {
        spdlog::debug("Initializing UDP server...");
        m_udp_server = std::make_unique<UdpServer>(
            m_config["udp_port"].get<uint16_t>(),
            m_session_manager,
            m_log
        );
        spdlog::info("UDP server initialized (port: {})", 
                     m_config["udp_port"].get<uint16_t>());
    } catch (const std::exception& e) {
        spdlog::error("UDP server initialization failed: {}", e.what());
        throw std::runtime_error("Cannot initialize UDP server: " + std::string(e.what()));
    }
}

void Core::initHttpServer() {
    try {
        spdlog::debug("Initializing HTTP server...");
        m_http_server = std::make_unique<HttpServer>(
            m_config["http_port"].get<uint16_t>(),
            m_session_manager,
            m_log
        );
        spdlog::info("HTTP server initialized (port: {})", 
                     m_config["http_port"].get<uint16_t>());
    } catch (const std::exception& e) {
        spdlog::error("HTTP server initialization failed: {}", e.what());
        throw std::runtime_error("Cannot initialize HTTP server: " + std::string(e.what()));
    }
}
