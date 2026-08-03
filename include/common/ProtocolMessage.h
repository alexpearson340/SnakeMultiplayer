#pragma once

#include "common/Json.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>

using Bytes = std::string;

enum class MessageType : int32_t {
    SERVER_CONFIG = 0,     // contains parameters such as the random seed used
    CLIENT_JOIN = 1,       // client to server, introduces the client
    CLIENT_DISCONNECT = 2, // end of contact between client and server
    SERVER_WELCOME = 3,    // server acknowledgement of client join
    CLIENT_INPUT = 4,      // client input of actions to server
    GAME_STATE = 5         // server broadcast of game state out to clients
};

struct ProtocolMessage {
    MessageType messageType;
    Bytes message;
    int clientId {-1};
    int64_t sequence {-1};
    int64_t transactTime {-1};
};

namespace jsonprotocol {

    inline Bytes toString(const ProtocolMessage & msg) {
        json j {{"message_type", static_cast<int>(msg.messageType)},
                {"message", msg.message},
                {"client_id", msg.clientId},
                {"sequence", msg.sequence},
                {"transact_time", msg.transactTime}};
        return j.dump();
    }

    inline ProtocolMessage fromString(const Bytes & str) {
        json j = json::parse(str);
        return {static_cast<MessageType>(j["message_type"]), j["message"], j["client_id"],
                static_cast<int64_t>(j["sequence"]), static_cast<int64_t>(j["transact_time"])};
    }

    inline ProtocolMessage fromString(const Bytes & str, int clientId) {
        ProtocolMessage pm {fromString(str)};
        pm.clientId = clientId;
        return pm;
    };

}; // namespace jsonprotocol

namespace protocol {
    struct ServerConfig;
    struct ClientInput;
    struct ClientDisconnect;
    struct ServerWelcome;
    struct ClientJoin;
    using MessageVariant = std::variant<ServerConfig, ClientInput, ClientDisconnect, ServerWelcome, ClientJoin>;

    // ProtocolMessage shim toString and fromString - until we fully remove ProtocolMessage
    inline Bytes toString(const ProtocolMessage & msg);

    inline ProtocolMessage fromString(const Bytes & str);

    inline ProtocolMessage fromString(const Bytes & str, int clientId);

    struct Header {
        MessageType messageType;
        int32_t clientId {-1};
        int64_t sequence {-1};
        int64_t transactTime {-1};
    };
    static_assert(offsetof(Header, messageType) == 0);
    static_assert(offsetof(Header, clientId) == 4);
    static_assert(offsetof(Header, sequence) == 8);
    static_assert(offsetof(Header, transactTime) == 16);
    static_assert(sizeof(Header) == 24);
    constexpr size_t HEADER_PACKED_SIZE {sizeof(Header::messageType) + sizeof(Header::clientId) +
        sizeof(Header::sequence) + sizeof(Header::transactTime)};
    
    struct ServerConfig {
        Header hdr;
        int32_t width;
        int32_t height;
        uint32_t seed;
        int32_t minFoodInArena;
        int32_t foodSpawnFromBodySegmentProbability;
        int32_t speedBoostProbability;
        float speedBoostRatio;
        int64_t movementFrequencyMs;
        int64_t boostedMovementFrequencyMs;
        int64_t boostDurationMs;
    };
    static_assert(alignof(Header) == 8);
    static_assert(offsetof(ServerConfig, width) == 24);
    static_assert(offsetof(ServerConfig, height) == 28);
    static_assert(offsetof(ServerConfig, seed) == 32);
    static_assert(offsetof(ServerConfig, minFoodInArena) == 36);
    static_assert(offsetof(ServerConfig, foodSpawnFromBodySegmentProbability) == 40);
    static_assert(offsetof(ServerConfig, speedBoostProbability) == 44);
    static_assert(offsetof(ServerConfig, speedBoostRatio) == 48);
    static_assert(offsetof(ServerConfig, movementFrequencyMs) == 56);
    static_assert(offsetof(ServerConfig, boostedMovementFrequencyMs) == 64);
    static_assert(offsetof(ServerConfig, boostDurationMs) == 72);
    static_assert(sizeof(ServerConfig) == 80);
    constexpr size_t SERVER_CONFIG_PACKED_SIZE {HEADER_PACKED_SIZE +
        sizeof(ServerConfig::width) + sizeof(ServerConfig::height) + sizeof(ServerConfig::seed) +
        sizeof(ServerConfig::minFoodInArena) + sizeof(ServerConfig::foodSpawnFromBodySegmentProbability) +
        sizeof(ServerConfig::speedBoostProbability) + sizeof(ServerConfig::speedBoostRatio) +
        sizeof(ServerConfig::movementFrequencyMs) + sizeof(ServerConfig::boostedMovementFrequencyMs) +
        sizeof(ServerConfig::boostDurationMs)};

    struct ClientInput {
        Header hdr;
        char input;
    };
    static_assert(offsetof(ClientInput, input) == 24);
    static_assert(sizeof(ClientInput) == 32);
    constexpr size_t CLIENT_INPUT_PACKED_SIZE {HEADER_PACKED_SIZE + sizeof(ClientInput::input)};

    struct ClientDisconnect {
        Header hdr;
    };
    static_assert(sizeof(ClientDisconnect) == 24);
    constexpr size_t CLIENT_DISCONNECT_PACKED_SIZE {HEADER_PACKED_SIZE};

    struct ServerWelcome {
        Header hdr;
    };
    static_assert(sizeof(ServerWelcome) == 24);
    constexpr size_t SERVER_WELCOME_PACKED_SIZE {HEADER_PACKED_SIZE};

    struct ClientJoin {
        Header hdr;
        char username[16];
    };
    static_assert(offsetof(ClientJoin, username) == 24);
    static_assert(sizeof(ClientJoin) == 40);
    constexpr size_t CLIENT_JOIN_PACKED_SIZE {HEADER_PACKED_SIZE + sizeof(ClientJoin::username)};

    // host native endianess is assumed - sending and receiving little endian
    template <typename T>
    inline void readRawBytes(const char * & source, T & dest) {
        static_assert(std::is_trivially_copyable_v<T>);
        std::memcpy(&dest, source, sizeof(dest));
        source += sizeof(dest);
    }

    template <typename T>
    inline void writeRawBytes(const T & source, char * & dest) {
        static_assert(std::is_trivially_copyable_v<T>);
        std::memcpy(dest, &source, sizeof(source));
        dest += sizeof(source);
    }

    inline Bytes serialiseHeader(const Header & msg) {
        Bytes buf;
        buf.resize(HEADER_PACKED_SIZE);
        char * raw = buf.data();
        writeRawBytes(msg.messageType, raw);
        writeRawBytes(msg.clientId, raw);
        writeRawBytes(msg.sequence, raw);
        writeRawBytes(msg.transactTime, raw);
        return buf;
    }

    inline Header deserialiseHeader(const Bytes & buf) {
        const char * raw = buf.data();
        Header msg;
        readRawBytes(raw, msg.messageType);
        readRawBytes(raw, msg.clientId);
        readRawBytes(raw, msg.sequence);
        readRawBytes(raw, msg.transactTime);
        return msg;
    }

    template <typename T>
    inline Bytes serialise(const T & msg) {
        Bytes buf {serialiseHeader(msg.hdr)};
        if constexpr (std::is_same_v<T, ClientJoin>) {
            buf.resize(CLIENT_JOIN_PACKED_SIZE);
            char * raw = buf.data() + HEADER_PACKED_SIZE;
            writeRawBytes(msg.username, raw);
            return buf;
        }
        else if constexpr (std::is_same_v<T, ClientDisconnect>) {
            return buf;
        }
        else if constexpr (std::is_same_v<T, ServerWelcome>) {
            return buf;
        }
        else if constexpr (std::is_same_v<T, ClientInput>) {
            buf.resize(CLIENT_INPUT_PACKED_SIZE);
            char * raw = buf.data() + HEADER_PACKED_SIZE;
            writeRawBytes(msg.input, raw);
            return buf;
        }
        else if constexpr (std::is_same_v<T, ServerConfig>) {
            buf.resize(SERVER_CONFIG_PACKED_SIZE);
            char * raw = buf.data() + HEADER_PACKED_SIZE;
            writeRawBytes(msg.width, raw);
            writeRawBytes(msg.height, raw);
            writeRawBytes(msg.seed, raw);
            writeRawBytes(msg.minFoodInArena, raw);
            writeRawBytes(msg.foodSpawnFromBodySegmentProbability, raw);
            writeRawBytes(msg.speedBoostProbability, raw);
            writeRawBytes(msg.speedBoostRatio, raw);
            writeRawBytes(msg.movementFrequencyMs, raw);
            writeRawBytes(msg.boostedMovementFrequencyMs, raw);
            writeRawBytes(msg.boostDurationMs, raw);
            return buf;
        }
        else {
            throw std::runtime_error("Invalid MessageType");
        }
    }

    inline MessageVariant deserialise(const Bytes & buf) {
        Header hdr;
        hdr = deserialiseHeader(buf);
        const char * raw = buf.data() + HEADER_PACKED_SIZE;

        switch (hdr.messageType) {
        case MessageType::CLIENT_JOIN: {
            assert(buf.size() == CLIENT_JOIN_PACKED_SIZE);
            ClientJoin msg;
            msg.hdr = hdr;
            readRawBytes(raw, msg.username);
            return msg;
        }
        case MessageType::CLIENT_DISCONNECT: {
            assert(buf.size() == CLIENT_DISCONNECT_PACKED_SIZE);
            ClientDisconnect msg;
            msg.hdr = hdr;
            return msg;
        }
        case MessageType::SERVER_WELCOME: {
            assert(buf.size() == SERVER_WELCOME_PACKED_SIZE);
            ServerWelcome msg;
            msg.hdr = hdr;
            return msg;
        }
        case MessageType::CLIENT_INPUT: {
            assert(buf.size() == CLIENT_INPUT_PACKED_SIZE);
            ClientInput msg;
            msg.hdr = hdr;
            readRawBytes(raw, msg.input);
            return msg;
        }
        case MessageType::SERVER_CONFIG: {
            assert(buf.size() == SERVER_CONFIG_PACKED_SIZE);
            ServerConfig msg;
            msg.hdr = hdr;
            readRawBytes(raw, msg.width);
            readRawBytes(raw, msg.height);
            readRawBytes(raw, msg.seed);
            readRawBytes(raw, msg.minFoodInArena);
            readRawBytes(raw, msg.foodSpawnFromBodySegmentProbability);
            readRawBytes(raw, msg.speedBoostProbability);
            readRawBytes(raw, msg.speedBoostRatio);
            readRawBytes(raw, msg.movementFrequencyMs);
            readRawBytes(raw, msg.boostedMovementFrequencyMs);
            readRawBytes(raw, msg.boostDurationMs);
            return msg;
        }
        default:
            throw std::runtime_error("Invalid MessageType");
        }
    }

    // Send-side shim: ProtocolMessage -> typed struct -> binary bytes. Types not yet
    // migrated (SERVER_CONFIG, GAME_STATE) fall back to the json encoding.
    inline Bytes toString(const ProtocolMessage & msg) {
        Header hdr {msg.messageType, msg.clientId, msg.sequence, msg.transactTime};
        switch (msg.messageType) {
        case MessageType::CLIENT_JOIN: {
            ClientJoin out {};
            out.hdr = hdr;
            std::strncpy(out.username, msg.message.c_str(), sizeof(out.username));
            return serialise(out);
        }
        case MessageType::CLIENT_DISCONNECT: {
            ClientDisconnect out;
            out.hdr = hdr;
            return serialise(out);
        }
        case MessageType::SERVER_WELCOME: {
            ServerWelcome out;
            out.hdr = hdr;
            return serialise(out);
        }
        case MessageType::CLIENT_INPUT: {
            ClientInput out;
            out.hdr = hdr;
            out.input = msg.message[0];
            return serialise(out);
        }
        case MessageType::SERVER_CONFIG:
        case MessageType::GAME_STATE:
            return jsonprotocol::toString(msg);
        default:
            throw std::runtime_error("Invalid MessageType");
        }
    }

    // Receive-side shim: binary/json frame -> ProtocolMessage. json frames start with '{';
    // binary frames start with a little-endian messageType int32 (byte 0 in 0x00..0x05),
    // so peek byte 0 to demux while encodings are mixed.
    inline ProtocolMessage fromString(const Bytes & str) {
        if (str[0] == '{') {
            return jsonprotocol::fromString(str);
        }
        Header hdr {deserialiseHeader(str)};
        switch (hdr.messageType) {
        case MessageType::CLIENT_JOIN: {
            const ClientJoin m {std::get<ClientJoin>(deserialise(str))};
            return {hdr.messageType, Bytes(m.username, strnlen(m.username, sizeof(m.username))),
                    hdr.clientId, hdr.sequence, hdr.transactTime};
        }
        case MessageType::CLIENT_INPUT: {
            const ClientInput m {std::get<ClientInput>(deserialise(str))};
            return {hdr.messageType, Bytes(1, m.input), hdr.clientId, hdr.sequence, hdr.transactTime};
        }
        case MessageType::CLIENT_DISCONNECT:
        case MessageType::SERVER_WELCOME:
            return {hdr.messageType, "", hdr.clientId, hdr.sequence, hdr.transactTime};
        default:
            throw std::runtime_error("Invalid MessageType");
        }
    }

    inline ProtocolMessage fromString(const Bytes & str, int clientId) {
        ProtocolMessage pm {fromString(str)};
        pm.clientId = clientId;
        return pm;
    }

}; // namespace protocol
