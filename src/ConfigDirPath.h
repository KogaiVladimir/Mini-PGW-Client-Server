//ConfigDirPath.h

#pragma once
#include <string>

namespace configDirPath {

    inline std::string serverLogFile() {
        return "../../log/server.log";
    }

    inline std::string clientLogFile() {
        return "../../log/client.log";
    }

    inline std::string clientConfig() {
        return "../../configs/client.json";
    }

    inline std::string serverConfig() {
        return "../../configs/server.json";
    }

};