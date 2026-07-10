#include "common/Constants.h"
#include "common/ProtocolMessage.h"
#include "snake_server/ServerConfig.h"

#include <gtest/gtest.h>
#include <optional>

TEST(InitServerConfig, UsesSeedFromHeader) {
    ProtocolMessage msg {MessageType::SERVER_CONFIG, R"({"seed":42})"};
    ServerConfig cfg {initServerConfig("test", msg)};
    EXPECT_EQ(cfg.seed, 42u);
}

TEST(InitServerConfig, FallsBackToConstantsWhenNoHeader) {
    ServerConfig cfg {initServerConfig("test", std::nullopt)};
    EXPECT_EQ(cfg.applicationName, "test");
    EXPECT_EQ(cfg.port, SERVER_PORT);
    EXPECT_EQ(cfg.width, ARENA_WIDTH);
    EXPECT_EQ(cfg.height, ARENA_HEIGHT);
}
