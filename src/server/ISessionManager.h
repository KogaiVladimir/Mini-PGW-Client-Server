//SessionManager.h

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <atomic>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>
#include "../Logger.h"

class ISessionManager {

public:

    virtual ~ISessionManager() = default;

    virtual std::string handleImsi(const std::string& imsi) = 0;

    virtual bool isSessionActive(const std::string& imsi) const = 0;

    virtual void startCleanupTimer() = 0;

    virtual void stopCleanupTimer() = 0;

    virtual void cleanupExpiredSessions() = 0;

    virtual void gracefulShutdown() = 0;

    virtual void addSession(const std::string& imsi) = 0;

    virtual void removeSession(const std::string& imsi) = 0;

    virtual void writeToCdr(const std::string& imsi, const std::string& action) = 0;

    virtual bool isBlacklisted(const std::string& imsi) const = 0;

    virtual std::string validImsi(const std::string& raw_imsi) const = 0;

};
