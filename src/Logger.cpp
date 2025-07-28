//Logger.cpp

#include "Logger.h"

Logger::Logger(const std::string& log_file_path)
: m_running(false) {
    m_log.open(log_file_path, std::ios::app);
    if (!m_log.is_open()) {
        throw std::runtime_error("Failed to open log file: " + log_file_path);
    }
}

Logger::~Logger() {
    stop();
}

void Logger::start() {
    if (m_running) return;
    m_running = true;
    m_write_thread = std::thread(&Logger::processMessages, this);
}

void Logger::stop() {
    m_running = false;
    m_condition.notify_one();
    
    if (m_write_thread.joinable()) {
        m_write_thread.join();
    }
    
    if (m_log.is_open()) {
        m_log.flush();
        m_log.close();
    }
}

void Logger::flush() {
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    if (!m_message_queue.empty()) {
        lock.unlock();
        m_condition.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Logger::sendToLog(const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_message_queue.push(message);
    }
    m_condition.notify_one();
}

void Logger::writeToFile(const std::string &message) {
    if (!m_log.is_open()) return;
    try {
        const auto now = std::chrono::system_clock::now();
        const auto now_time = std::chrono::system_clock::to_time_t(now);
        char timestamp[20];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
        m_log << "[" << timestamp << "] " << message << std::endl;
    } catch (...) {
        spdlog::error("Failed to write log message");
    }
}

void Logger::processMessages() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        
        if (!m_running) break;

        // Ждем сообщений или остановки
        m_condition.wait_for(lock, std::chrono::seconds(1), [this]() {
            return !m_message_queue.empty() || !m_running;
        });
        
        // Если есть сообщения - обрабатываем всю очередь
        while (!m_message_queue.empty()) {
            const std::string message = std::move(m_message_queue.front());
            m_message_queue.pop();
            
            lock.unlock(); // Разблокируем на время записи
            writeToFile(message);
            lock.lock(); // Снова блокируем для проверки очереди
        }
        
        // Принудительная запись на диск
        if (m_log.is_open()) m_log.flush();
    }
    
    // Записываем оставшиеся сообщения перед выходом
    while (!m_message_queue.empty()) {
        const std::string message = m_message_queue.front();
        m_message_queue.pop();
        writeToFile(message);
    }
}
