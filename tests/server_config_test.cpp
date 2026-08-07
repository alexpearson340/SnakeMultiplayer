#include "common/Constants.h"
#include "common/Protocol.h"
#include "snake_server/ServerConfig.h"

#include <gtest/gtest.h>
#include <optional>

TEST(InitServerConfig, UsesSeedFromHeader) {
    protocol::ServerConfig sc {};
    sc.hdr.messageType = protocol::MessageType::SERVER_CONFIG;
    sc.seed = 42u;
    ServerConfig cfg {initServerConfig("test", protocol::MessageVariant {sc})};
    EXPECT_EQ(cfg.seed, 42u);
}

TEST(InitServerConfig, FallsBackToConstantsWhenNoHeader) {
    ServerConfig cfg {initServerConfig("test", std::nullopt)};
    EXPECT_EQ(cfg.applicationName, "test");
    EXPECT_EQ(cfg.port, SERVER_PORT);
    EXPECT_EQ(cfg.width, ARENA_WIDTH);
    EXPECT_EQ(cfg.height, ARENA_HEIGHT);
}
