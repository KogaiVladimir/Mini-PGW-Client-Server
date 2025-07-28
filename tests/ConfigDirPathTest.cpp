//ConfigDirPathTest.cpp

#include <gtest/gtest.h>
#include "../src/ConfigDirPath.h"

TEST(ConfigDirPathTest, ServerLogPath) {
    EXPECT_EQ(configDirPath::serverLogFile(), "../../log/server.log");
}

TEST(ConfigDirPathTest, ClientLogPath) {
    EXPECT_EQ(configDirPath::clientLogFile(), "../../log/client.log");
}

TEST(ConfigDirPathTest, ServerConfigPath) {
    EXPECT_EQ(configDirPath::serverConfig(), "../../configs/server.json");
}

TEST(ConfigDirPathTest, ClientConfigPath) {
    EXPECT_EQ(configDirPath::clientConfig(), "../../configs/client.json");
}