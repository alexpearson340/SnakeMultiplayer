#pragma once

#include <cassert>
#include <chrono>
#include <cstdint>
#include <optional>
#include <random>
#include <string>

#include "common/Constants.h"
#include "common/Json.h"
#include "common/ProtocolMessage.h"

struct ServerConfig {
    const std::string applicationName;
    const int port;
    const int width;
    const int height;
    const std::uint32_t seed;   
    const std::chrono::milliseconds movementFrequencyMs;
    const std::chrono::milliseconds boostedMovementFrequencyMs;
    const std::chrono::milliseconds boostDurationMs;
};

inline ServerConfig initServerConfig(const std::string & applicationName, const std::optional<ProtocolMessage> & msg) {
    json j = json::object();
    if (msg.has_value()) {
        assert(msg->messageType == MessageType::SERVER_CONFIG &&
               "initServerConfig ProtocolMessage must be type SERVER_CONFIG");
        j = json::parse(msg->message);
    }
    return ServerConfig {
        .applicationName = applicationName,
        .port = SERVER_PORT,
        .width = ARENA_WIDTH,
        .height = ARENA_HEIGHT,
        .seed = j.value("seed", std::random_device {}()),
        .movementFrequencyMs = std::chrono::milliseconds(MOVEMENT_FREQUENCY_MS),
        .boostedMovementFrequencyMs = std::chrono::milliseconds(BOOSTED_MOVEMENT_FREQUENCY_MS),
        .boostDurationMs = std::chrono::milliseconds(SPEED_BOOST_DURATION_MS),
    };
};