//SessionManager.cpp

#include "SessionManager.h"

SessionManager::SessionManager(
    const uint16_t& session_timeout_sec,
    const uint16_t& graceful_shutdown_rate,
    const std::string& cdr_file_path,
    const std::vector<std::string>& blacklist,
    std::shared_ptr<Logger> log)
    : m_shutting_down(false), m_session_timeout_sec(session_timeout_sec),
    m_graceful_shutdown_rate(graceful_shutdown_rate),
    m_blacklist(blacklist), m_log(log), m_cleanup_running(false) {
    m_cdr_file.open(cdr_file_path, std::ios::app);
    if (!m_cdr_file.is_open()) {
        spdlog::error("Failed to open CDR file: {}", cdr_file_path);
        throw std::runtime_error("CDR file error");
    }
}

SessionManager::~SessionManager() {
    gracefulShutdown();
}

std::string SessionManager::handleImsi(const std::string& raw_imsi) {
    std::string imsi = validImsi(raw_imsi);
    
    if (imsi.empty()) return "rejected";

    if (m_shutting_down) return "rejected (server shutting down)";

    std::lock_guard<std::mutex> lock(m_mutex);

    if (isBlacklisted(imsi)) {
        writeToCdr(imsi, "rejected_blacklist");
        m_log->sendToLog("Session rejected for IMSI: " + imsi);
        spdlog::info("Session rejected for IMSI: {}", imsi);
        return "rejected";
    }

    auto it = m_sessions.find(imsi);
    if (it != m_sessions.end()) {
        it->second.created_at = std::chrono::system_clock::now();
        return "exists";
    }

    addSession(imsi);
    return "created";
}

bool SessionManager::isSessionActive(const std::string &imsi) const {
    std::cout << "SessionManager::isSessionActive:IMSI: " << imsi << std::endl;
    spdlog::debug("SessionManager::isSessionActive:IMSI: {}", imsi);
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(imsi);

    if (it != m_sessions.end()) {
        std::cout << "SessionManager::isSessionActive:it->second.active: "
            << it->second.active << std::endl;
    } else if (it == m_sessions.end()) {
        std::cout << "it == m_sessions.end() " << std::endl;
    }

    return it != m_sessions.end() && it->second.active;
}

void SessionManager::startCleanupTimer() {
    m_cleanup_running = true;
    m_cleanup_thread = std::thread([this]() {
        while (m_cleanup_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            cleanupExpiredSessions();
        }
    });
}

void SessionManager::stopCleanupTimer() {
    m_cleanup_running = false;
    if (m_cleanup_thread.joinable()) {
        m_cleanup_thread.join();
    }
}

void SessionManager::gracefulShutdown() {
    m_log->sendToLog("Starting graceful shutdown...");
    stopCleanupTimer();
    
    while (!m_sessions.empty()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.begin();
        writeToCdr(it->first, "shutdown_remove");
        m_sessions.erase(it);
        std::this_thread::sleep_for(
            std::chrono::milliseconds(m_graceful_shutdown_rate)
        );
    }
    if (m_cdr_file.is_open()) m_cdr_file.close();
}

void SessionManager::addSession(const std::string& imsi) {
    std::cout << "SessionManager::addSession:IMSI: " << imsi << std::endl;
    m_sessions[imsi] = Session{
        .created_at = std::chrono::system_clock::now(),
        .active = true
    };
    writeToCdr(imsi, "created");
    m_log->sendToLog("Created: " + imsi);
    spdlog::info("Session created for IMSI: {}", imsi);
}

void SessionManager::removeSession(const std::string& imsi) {
    if (m_sessions.erase(imsi)) {
        writeToCdr(imsi, "timeout_remove");
        m_log->sendToLog("Timeout remove IMSI: " + imsi);
    }
}

void SessionManager::writeToCdr(const std::string& imsi, const std::string& action) {
    static std::mutex cdr_mutex;
    std::lock_guard<std::mutex> lock(cdr_mutex);
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
    m_cdr_file << timestamp << ", " << imsi << ", " << action << std::endl;
}

bool SessionManager::isBlacklisted(const std::string& imsi) const {
    return std::find(m_blacklist.begin(), m_blacklist.end(), imsi) != m_blacklist.end();
}

void SessionManager::cleanupExpiredSessions() {
    auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.created_at).count();
            
        if (duration > m_session_timeout_sec) {
            writeToCdr(it->first, "timeout_remove"); 
            m_log->sendToLog("Timeout remove IMSI: " + it->first);
            it = m_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

std::string SessionManager::validImsi(const std::string& raw_imsi) const {
    std::string clean_imsi;
    std::copy_if(raw_imsi.begin(), raw_imsi.end(), 
                std::back_inserter(clean_imsi),
                [](char c) { return std::isdigit(c); });
    
    spdlog::debug("Valid IMSI: '{}' -> '{}'", raw_imsi, clean_imsi);
    return clean_imsi.size() <= 15 ? clean_imsi : "";
}