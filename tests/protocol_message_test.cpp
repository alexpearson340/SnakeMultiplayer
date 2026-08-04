#include "common/ProtocolMessage.h"

#include <gtest/gtest.h>

TEST(ProtocolBinary, HeaderRoundTrip) {
    const protocol::Header original {MessageType::CLIENT_INPUT, 7, 123456789, 987654321012345};

    const protocol::Header decoded {protocol::deserialiseHeader(protocol::serialiseHeader(original))};

    EXPECT_EQ(decoded.messageType, original.messageType);
    EXPECT_EQ(decoded.clientId, original.clientId);
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_EQ(decoded.transactTime, original.transactTime);
}

TEST(ProtocolBinary, ClientInputRoundTrip) {
    const protocol::ClientInput original {{MessageType::CLIENT_INPUT, 7, 123456789, 987654321012345}, 'w'};

    const protocol::ClientInput decoded {
        std::get<protocol::ClientInput>(protocol::deserialise(protocol::serialise(original)))};

    EXPECT_EQ(decoded.hdr.messageType, original.hdr.messageType);
    EXPECT_EQ(decoded.hdr.clientId, original.hdr.clientId);
    EXPECT_EQ(decoded.hdr.sequence, original.hdr.sequence);
    EXPECT_EQ(decoded.hdr.transactTime, original.hdr.transactTime);
    EXPECT_EQ(decoded.input, original.input);
}

TEST(ProtocolBinary, ClientJoinRoundTrip) {
    const protocol::ClientJoin original {{MessageType::CLIENT_JOIN, 7, 123456789, 987654321012345}, "alexpearson"};

    const protocol::ClientJoin decoded {
        std::get<protocol::ClientJoin>(protocol::deserialise(protocol::serialise(original)))};

    EXPECT_EQ(decoded.hdr.messageType, original.hdr.messageType);
    EXPECT_EQ(decoded.hdr.clientId, original.hdr.clientId);
    EXPECT_EQ(decoded.hdr.sequence, original.hdr.sequence);
    EXPECT_EQ(decoded.hdr.transactTime, original.hdr.transactTime);
    EXPECT_STREQ(decoded.username, original.username);
}

TEST(ProtocolBinary, ClientDisconnectRoundTrip) {
    const protocol::ClientDisconnect original {{MessageType::CLIENT_DISCONNECT, 7, 123456789, 987654321012345}};

    const protocol::ClientDisconnect decoded {
        std::get<protocol::ClientDisconnect>(protocol::deserialise(protocol::serialise(original)))};

    EXPECT_EQ(decoded.hdr.messageType, original.hdr.messageType);
    EXPECT_EQ(decoded.hdr.clientId, original.hdr.clientId);
    EXPECT_EQ(decoded.hdr.sequence, original.hdr.sequence);
    EXPECT_EQ(decoded.hdr.transactTime, original.hdr.transactTime);
}

TEST(ProtocolBinary, ServerWelcomeRoundTrip) {
    const protocol::ServerWelcome original {{MessageType::SERVER_WELCOME, 7, 123456789, 987654321012345}};

    const protocol::ServerWelcome decoded {
        std::get<protocol::ServerWelcome>(protocol::deserialise(protocol::serialise(original)))};

    EXPECT_EQ(decoded.hdr.messageType, original.hdr.messageType);
    EXPECT_EQ(decoded.hdr.clientId, original.hdr.clientId);
    EXPECT_EQ(decoded.hdr.sequence, original.hdr.sequence);
    EXPECT_EQ(decoded.hdr.transactTime, original.hdr.transactTime);
}

TEST(ProtocolBinary, ServerConfigRoundTrip) {
    const protocol::ServerConfig original {
        {MessageType::SERVER_CONFIG, 7, 123456789, 987654321012345},
        40, 40, 4246754215u, 6, 3, 80, 1.5f, 200, 133, 8000};

    const protocol::ServerConfig decoded {
        std::get<protocol::ServerConfig>(protocol::deserialise(protocol::serialise(original)))};

    EXPECT_EQ(decoded.hdr.messageType, original.hdr.messageType);
    EXPECT_EQ(decoded.hdr.clientId, original.hdr.clientId);
    EXPECT_EQ(decoded.hdr.sequence, original.hdr.sequence);
    EXPECT_EQ(decoded.hdr.transactTime, original.hdr.transactTime);
    EXPECT_EQ(decoded.width, original.width);
    EXPECT_EQ(decoded.height, original.height);
    EXPECT_EQ(decoded.seed, original.seed);
    EXPECT_EQ(decoded.minFoodInArena, original.minFoodInArena);
    EXPECT_EQ(decoded.foodSpawnFromBodySegmentProbability, original.foodSpawnFromBodySegmentProbability);
    EXPECT_EQ(decoded.speedBoostProbability, original.speedBoostProbability);
    EXPECT_EQ(decoded.speedBoostRatio, original.speedBoostRatio);
    EXPECT_EQ(decoded.movementFrequencyMs, original.movementFrequencyMs);
    EXPECT_EQ(decoded.boostedMovementFrequencyMs, original.boostedMovementFrequencyMs);
    EXPECT_EQ(decoded.boostDurationMs, original.boostDurationMs);
}

namespace {

    void expectFoodEq(const protocol::GameState::Food & a, const protocol::GameState::Food & b) {
        EXPECT_EQ(a.colour, b.colour);
        EXPECT_EQ(a.icon, b.icon);
        EXPECT_EQ(a.x, b.x);
        EXPECT_EQ(a.y, b.y);
    }

    void expectGameStateEq(const protocol::GameState & d, const protocol::GameState & o) {
        EXPECT_EQ(d.hdr.messageType, o.hdr.messageType);
        EXPECT_EQ(d.hdr.clientId, o.hdr.clientId);
        EXPECT_EQ(d.hdr.sequence, o.hdr.sequence);
        EXPECT_EQ(d.hdr.transactTime, o.hdr.transactTime);
        EXPECT_EQ(d.highScore, o.highScore);
        EXPECT_EQ(std::memcmp(d.highScoreUsername, o.highScoreUsername, sizeof(o.highScoreUsername)), 0);

        ASSERT_EQ(d.food.size(), o.food.size());
        for (size_t i {0}; i < o.food.size(); ++i) {
            expectFoodEq(d.food[i], o.food[i]);
        }

        ASSERT_EQ(d.speedBoosts.size(), o.speedBoosts.size());
        for (size_t i {0}; i < o.speedBoosts.size(); ++i) {
            expectFoodEq(d.speedBoosts[i], o.speedBoosts[i]);
        }

        ASSERT_EQ(d.players.size(), o.players.size());
        for (size_t i {0}; i < o.players.size(); ++i) {
            const protocol::GameState::Player & dp {d.players[i]};
            const protocol::GameState::Player & op {o.players[i]};
            EXPECT_EQ(dp.clientId, op.clientId);
            EXPECT_EQ(dp.colour, op.colour);
            EXPECT_EQ(dp.direction, op.direction);
            EXPECT_EQ(dp.score, op.score);
            EXPECT_EQ(std::memcmp(dp.username, op.username, sizeof(op.username)), 0);
            EXPECT_EQ(dp.segments, op.segments);
        }
    }

    protocol::GameState gameStateRoundTrip(const protocol::GameState & original) {
        return std::get<protocol::GameState>(protocol::deserialise(protocol::serialise(original)));
    }

} // namespace

TEST(ProtocolBinary, GameStateAllEmpty) {
    const protocol::GameState original {
        {MessageType::GAME_STATE, -1, 123456789, 987654321012345}, 8, "bot", {}, {}, {}};
    expectGameStateEq(gameStateRoundTrip(original), original);
}

TEST(ProtocolBinary, GameStateFoodOnly) {
    const protocol::GameState original {
        {MessageType::GAME_STATE, -1, 123456789, 987654321012345}, 8, "bot",
        {{3, '@', 2, 18}, {1, '@', 24, 8}}, {}, {}};
    expectGameStateEq(gameStateRoundTrip(original), original);
}

TEST(ProtocolBinary, GameStatePlayersNoFood) {
    const protocol::GameState original {
        {MessageType::GAME_STATE, -1, 123456789, 987654321012345}, 8, "bot", {}, {},
        {{7, 3, '>', 5, "alice", {{26, 22}, {26, 23}}}}};
    expectGameStateEq(gameStateRoundTrip(original), original);
}

TEST(ProtocolBinary, GameStateFullMixedSegments) {
    const protocol::GameState original {
        {MessageType::GAME_STATE, -1, 123456789, 987654321012345}, 8, "bot",
        {{3, '@', 2, 18}},
        {{5, '*', 10, 12}, {6, '*', 4, 4}},
        {{7, 3, '>', 5, "alice", {{26, 22}, {26, 23}}},
         {9, 4, '^', 0, "bob", {}}}};
    expectGameStateEq(gameStateRoundTrip(original), original);
}

TEST(ProtocolShim, ServerConfigRoundTrip) {
    const ProtocolMessage original {MessageType::SERVER_CONFIG, "seed=42", 7, 123456789, 987654321012345};

    const ProtocolMessage decoded {protocol::fromString(protocol::toString(original))};

    EXPECT_EQ(decoded.messageType, original.messageType);
    EXPECT_EQ(decoded.message, original.message);
    EXPECT_EQ(decoded.clientId, original.clientId);
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_EQ(decoded.transactTime, original.transactTime);
}

TEST(ProtocolShim, ClientJoinRoundTrip) {
    const ProtocolMessage original {MessageType::CLIENT_JOIN, "alexpearson", 7, 123456789, 987654321012345};

    const ProtocolMessage decoded {protocol::fromString(protocol::toString(original))};

    EXPECT_EQ(decoded.messageType, original.messageType);
    EXPECT_EQ(decoded.message, original.message);
    EXPECT_EQ(decoded.clientId, original.clientId);
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_EQ(decoded.transactTime, original.transactTime);
}

TEST(ProtocolShim, ClientDisconnectRoundTrip) {
    const ProtocolMessage original {MessageType::CLIENT_DISCONNECT, "", 7, 123456789, 987654321012345};

    const ProtocolMessage decoded {protocol::fromString(protocol::toString(original))};

    EXPECT_EQ(decoded.messageType, original.messageType);
    EXPECT_EQ(decoded.message, original.message);
    EXPECT_EQ(decoded.clientId, original.clientId);
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_EQ(decoded.transactTime, original.transactTime);
}

TEST(ProtocolShim, ServerWelcomeRoundTrip) {
    const ProtocolMessage original {MessageType::SERVER_WELCOME, "", 7, 123456789, 987654321012345};

    const ProtocolMessage decoded {protocol::fromString(protocol::toString(original))};

    EXPECT_EQ(decoded.messageType, original.messageType);
    EXPECT_EQ(decoded.message, original.message);
    EXPECT_EQ(decoded.clientId, original.clientId);
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_EQ(decoded.transactTime, original.transactTime);
}

TEST(ProtocolShim, ClientInputRoundTrip) {
    const ProtocolMessage original {MessageType::CLIENT_INPUT, "w", 7, 123456789, 987654321012345};

    const ProtocolMessage decoded {protocol::fromString(protocol::toString(original))};

    EXPECT_EQ(decoded.messageType, original.messageType);
    EXPECT_EQ(decoded.message, original.message);
    EXPECT_EQ(decoded.clientId, original.clientId);
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_EQ(decoded.transactTime, original.transactTime);
}

TEST(ProtocolShim, GameStateRoundTrip) {
    const ProtocolMessage original {MessageType::GAME_STATE, "board", 7, 123456789, 987654321012345};

    const ProtocolMessage decoded {protocol::fromString(protocol::toString(original))};

    EXPECT_EQ(decoded.messageType, original.messageType);
    EXPECT_EQ(decoded.message, original.message);
    EXPECT_EQ(decoded.clientId, original.clientId);
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_EQ(decoded.transactTime, original.transactTime);
}
