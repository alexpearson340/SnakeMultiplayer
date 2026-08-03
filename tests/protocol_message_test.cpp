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
