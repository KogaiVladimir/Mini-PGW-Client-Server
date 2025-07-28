//main.cpp

#include "Core.h"

int main(int argc, char* argv[]) {
    try {
        // Инициализация логгера
        std::shared_ptr<Logger> logger = std::make_shared<Logger>(configDirPath::clientLogFile());

        // Создание экземпляра Core
        auto core = std::make_unique<Core>(logger);
        spdlog::info("Client application started");

        // Отправка запроса
        if (argc > 1) {
            std::string imsi = argv[1];
            core->sendRequest(imsi);
        } else {
            // Тестовый запрос если нет аргументов
            core->sendRequest("123456789012345");
            spdlog::warn("Using default test IMSI, specify real IMSI as argument");
        }

        return 0;
    } 
    catch (const std::exception& e) {
        spdlog::critical("Application error: {}", e.what());
        return 1;
    }
}