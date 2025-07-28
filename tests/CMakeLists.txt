#tests/CMakeLists.txt

add_executable(pgw_tests
    LoggerTest.cpp
    ConfigDirPathTest.cpp
    server_test/SessionManagerTest.cpp
    server_test/UdpServerTest.cpp
    server_test/HttpServerTest.cpp
    ../src/Logger.cpp
    ../src/server/SessionManager.cpp
    ../src/server/UdpServer.cpp
    ../src/server/HttpServer.cpp
    ../src/client/UdpClient.cpp
)

target_link_libraries(pgw_tests PRIVATE
    gtest
    gmock 
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    httplib
    gtest_main
)

target_include_directories(pgw_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

enable_testing()
add_test(NAME pgw_tests COMMAND pgw_tests)