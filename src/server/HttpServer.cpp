//HttpServer.cpp

#include "HttpServer.h"

HttpServer::HttpServer(const uint16_t& port, 
    std::shared_ptr<ISessionManager> session_manager,
    std::shared_ptr<Logger> log) 
    : m_port(port), 
      m_session_manager(session_manager), 
      m_log(log),
      m_server(std::make_unique<httplib::Server>()),
      m_running(false)
{
    m_log->sendToLog("HTTP Server instance created for port: " + std::to_string(m_port));
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (m_running) return;
    
    m_running = true;
    m_http_server_thread = std::thread([this]() {
        try {
            setupRoutes();
            m_log->sendToLog("HTTP server starting on port: " + std::to_string(m_port));
            m_server->listen("0.0.0.0", m_port);
            m_log->sendToLog("HTTP server stopped");
        } catch (const std::exception& e) {
            if (m_running) {
                m_log->sendToLog("HTTP server ERROR: " + std::string(e.what()));
            }
        }
    });
}

void HttpServer::stop() {
    if (!m_running) return;
    m_log->sendToLog("HTTP server stopping on port: " + std::to_string(m_port));
    spdlog::info("HTTP server stopping...");
        
    m_running = false;
    try {
        m_server->stop();
        if (m_http_server_thread.joinable()) {
            std::cout << "Joining server thread..." << std::endl;
            if (m_http_server_thread.get_id() != std::this_thread::get_id()) {
                m_http_server_thread.join();
            } else {
                m_http_server_thread.detach();
            }
        }        
    } catch (const std::exception& e) {
        m_log->sendToLog("Stop error: " + std::string(e.what()));
    }
}

void HttpServer::setupRoutes() {
    m_log->sendToLog("Setting up HTTP routes for port: " + std::to_string(m_port));

    m_server->Get("/check_subscriber", [this](const httplib::Request& req, httplib::Response& res) {
        std::string imsi = req.get_param_value("imsi");
        m_log->sendToLog("HTTP request /check_subscriber for IMSI: " + imsi);
        
        if (imsi.empty()) {
            m_log->sendToLog("HTTP 400: Missing IMSI parameter");
            res.status = 400;
            res.set_content("IMSI parameter is missing", "text/plain");
            return;
        }

        try {
            bool is_active = m_session_manager->isSessionActive(imsi);
            std::string status = is_active ? "active" : "not active";
            m_log->sendToLog("HTTP 200: Subscriber " + imsi + " status: " + status);
            res.status = 200;
            res.set_content(status, "text/plain");
        } catch (const std::exception& e) {
            m_log->sendToLog("HTTP 500: Error checking subscriber " + imsi + ": " + e.what());
            res.status = 500;
            res.set_content("Internal server error", "text/plain");
        }
    });

    m_server->Get("/stop", [this](const httplib::Request&, httplib::Response& res) {
        m_log->sendToLog("HTTP: Received shutdown command");
        spdlog::info("HTTP: Received stop command");
        res.status = 200;
        res.set_content("Server is shutting down...", "text/plain");
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Даём время на отправку
            this->stop();
        }).detach();
    });

    m_log->sendToLog("HTTP routes setup completed for port: " + std::to_string(m_port));
}