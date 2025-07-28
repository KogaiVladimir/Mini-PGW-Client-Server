//SessionManager.h

#pragma once

#include "ISessionManager.h"

struct Session {
    std::chrono::system_clock::time_point created_at;
    bool active;
};

class SessionManager : public ISessionManager {

public:

    SessionManager(
        const uint16_t& session_timeout_sec,
        const uint16_t& graceful_shutdown_rate,
        const std::string& cdr_file_path,
        const std::vector<std::string>& blacklist,
        std::shared_ptr<Logger> log
    );
    
    ~SessionManager();

    // Обработка IMSI: создание/отклонение сессии
    std::string handleImsi(const std::string& raw_imsi) final;

    // Проверка активности сессии
    bool isSessionActive(const std::string& imsi) const final;

    void startCleanupTimer() final;
    
    void stopCleanupTimer() final;

    void gracefulShutdown() final;

    void cleanupExpiredSessions() final;

private:

    void addSession(const std::string& imsi) final;

    void removeSession(const std::string& imsi) final;
    
    void writeToCdr(const std::string& imsi, const std::string& action) final;
    
    bool isBlacklisted(const std::string& imsi) const final;

    std::string validImsi(const std::string& raw_imsi) const final;

    //Глобальные переменныые

    std::unordered_map<std::string, Session> m_sessions;
        
    mutable std::mutex m_mutex;
    
    std::atomic<bool> m_shutting_down;
    
    uint16_t m_session_timeout_sec;

    uint16_t m_graceful_shutdown_rate;
    
    std::ofstream m_cdr_file;
    
    std::vector<std::string> m_blacklist;
    
    std::shared_ptr<Logger> m_log;

    std::atomic<bool> m_cleanup_running;
    
    std::thread m_cleanup_thread;
    
};
