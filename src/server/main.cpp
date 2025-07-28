//main.cpp

#include "Core.h"

int main() {
    spdlog::set_level(spdlog::level::debug);

    try {
        std::shared_ptr<Logger> log = std::make_shared<Logger>(configDirPath::serverLogFile());
        std::unique_ptr<Core> core = std::make_unique<Core>(log);
        core->start();

        while (core->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        spdlog::info("Server stopped gracefully");
        return 0;
    } catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());
        return 1;
    }
}