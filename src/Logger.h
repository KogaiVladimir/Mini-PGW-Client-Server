//Logger.h

#pragma once
#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <thread>
#include <memory>
#include <spdlog/spdlog.h>
#include "ConfigDirPath.h"

class Logger {

public:

    explicit Logger(const std::string& log_file_path);
    
    ~Logger();

    void start();

    void stop();

    void flush();

    // Отправить сообщение в лог (добавляет в очередь)
    void sendToLog(const std::string& message);

    // Записать сообщение непосредственно в файл
    void writeToFile(const std::string& message);

private:
    // Основной цикл обработки сообщений (работает в отдельном потоке)
    void processMessages();

    std::thread m_write_thread; // Поток для асинхронной записи
    
    std::ofstream m_log;// Файл лога
    
    std::queue<std::string> m_message_queue;// Очередь сообщений
    
    std::mutex m_queue_mutex;// Мьютекс для синхронизации доступа к очереди
    
    std::condition_variable m_condition;// Условная переменная для ожидания сообщений
    
    std::atomic<bool> m_running;// Флаг работы логгера
};