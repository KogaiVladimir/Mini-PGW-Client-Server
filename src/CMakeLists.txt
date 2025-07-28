#src/CMakeLists.txt

add_executable(pgw_server
    ConfigDirPath.h
    Logger.cpp
    Logger.h
    server/main.cpp
    server/Core.cpp
    server/Core.h
    server/UdpServer.cpp
    server/UdpServer.h
    server/HttpServer.cpp
    server/HttpServer.h
    server/ISessionManager.h
    server/SessionManager.cpp
    server/SessionManager.h
)

target_link_libraries(pgw_server PRIVATE 
    spdlog::spdlog 
    httplib 
    nlohmann_json::nlohmann_json
)

add_executable(pgw_client
    ConfigDirPath.h
    Logger.cpp
    Logger.h
    client/main.cpp
    client/Core.cpp
    client/Core.h
    client/UdpClient.cpp
    client/UdpClient.h
)

target_link_libraries(pgw_client PRIVATE
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

install(TARGETS pgw_server pgw_client
    RUNTIME DESTINATION bin
)