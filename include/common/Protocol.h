#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <type_traits>

using Bytes = std::string;

namespace protocol {
    
    enum class MessageType : int32_t {
        SERVER_CONFIG = 0,     // contains parameters such as the random seed used
        CLIENT_JOIN = 1,       // client to server, introduces the client
        CLIENT_DISCONNECT = 2, // end of contact between client and server
        SERVER_WELCOME = 3,    // server acknowledgement of client join
        CLIENT_INPUT = 4,      // client input of actions to server
        GAME_STATE = 5         // server broadcast of game state out to clients
    };

    struct ServerConfig;
    struct ClientInput;
    struct ClientDisconnect;
    struct ServerWelcome;
    struct ClientJoin;
    struct GameState;
    using MessageVariant =
        std::variant<ServerConfig, ClientInput, ClientDisconnect, ServerWelcome, ClientJoin, GameState>;

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
    constexpr size_t SERVER_CONFIG_PACKED_SIZE {
        HEADER_PACKED_SIZE + sizeof(ServerConfig::width) + sizeof(ServerConfig::height) + sizeof(ServerConfig::seed) +
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

    struct GameState {
        struct Food {
            int32_t color;
            char icon;
            int32_t x;
            int32_t y;
        };
        using SpeedBoost = Food;

        struct Player {
            using Segment = std::pair<int32_t, int32_t>;

            int32_t clientId;
            int32_t color;
            char direction;
            int32_t score;
            char username[16];
            std::vector<Segment> segments;
        };

        Header hdr;
        int32_t highScore;
        char highScoreUsername[16];
        std::vector<Food> food;
        std::vector<Food> speedBoosts;
        std::vector<Player> players;
    };
    static_assert(alignof(Header) == 8);
    static_assert(offsetof(GameState, highScore) == 24);
    static_assert(offsetof(GameState, highScoreUsername) == 28);
    static_assert(offsetof(GameState, food) == 48);
    static_assert(alignof(GameState::Food) == 4);
    static_assert(sizeof(GameState::SpeedBoost) == 16);
    static_assert(alignof(GameState::Player) == 8);
    static_assert(sizeof(GameState::Player::Segment) == 8);
    static_assert(offsetof(GameState::Player, clientId) == 0);
    static_assert(offsetof(GameState::Player, color) == 4);
    static_assert(offsetof(GameState::Player, direction) == 8);
    static_assert(offsetof(GameState::Player, score) == 12);
    static_assert(offsetof(GameState::Player, username) == 16);
    static_assert(offsetof(GameState::Player, segments) == 32);

    inline Header & header(MessageVariant & msg) {
        return std::visit([](auto & m) -> Header & {return m.hdr;}, msg);
    }

    inline const Header & header(const MessageVariant & msg) {
        return std::visit([](const auto & m) -> const Header & {return m.hdr;}, msg);
    }

    // host native endianess is assumed - sending and receiving little endian
    template <typename T>
    inline void readRawBytes(const char *& source, T & dest) {
        static_assert(std::is_trivially_copyable_v<T>);
        std::memcpy(&dest, source, sizeof(dest));
        source += sizeof(dest);
    }

    // use write for fixed size messages (destination buffer already sized)
    template <typename T>
    inline void writeRawBytes(const T & source, char *& dest) {
        static_assert(std::is_trivially_copyable_v<T>);
        std::memcpy(dest, &source, sizeof(source));
        dest += sizeof(source);
    }

    // use append for dynamically sized messages (destination buffer grows dynamically)
    template <typename T>
    inline void appendRawBytes(const T & source, Bytes & dest) {
        static_assert(std::is_trivially_copyable_v<T>);
        size_t old {dest.size()};
        dest.resize(old + sizeof(source));
        std::memcpy(dest.data() + old, &source, sizeof(source));
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
        } else if constexpr (std::is_same_v<T, ClientDisconnect>) {
            return buf;
        } else if constexpr (std::is_same_v<T, ServerWelcome>) {
            return buf;
        } else if constexpr (std::is_same_v<T, ClientInput>) {
            buf.resize(CLIENT_INPUT_PACKED_SIZE);
            char * raw = buf.data() + HEADER_PACKED_SIZE;
            writeRawBytes(msg.input, raw);
            return buf;
        } else if constexpr (std::is_same_v<T, ServerConfig>) {
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
        } else if constexpr (std::is_same_v<T, GameState>) {
            appendRawBytes(msg.highScore, buf);
            appendRawBytes(msg.highScoreUsername, buf);

            uint32_t foodCount {static_cast<uint32_t>(msg.food.size())};
            appendRawBytes(foodCount, buf);
            for (auto & f : msg.food) {
                appendRawBytes(f.color, buf);
                appendRawBytes(f.icon, buf);
                appendRawBytes(f.x, buf);
                appendRawBytes(f.y, buf);
            }

            uint32_t speedBoostCount {static_cast<uint32_t>(msg.speedBoosts.size())};
            appendRawBytes(speedBoostCount, buf);
            for (auto & sb : msg.speedBoosts) {
                appendRawBytes(sb.color, buf);
                appendRawBytes(sb.icon, buf);
                appendRawBytes(sb.x, buf);
                appendRawBytes(sb.y, buf);
            }

            uint32_t playerCount {static_cast<uint32_t>(msg.players.size())};
            uint32_t segmentCount;
            appendRawBytes(playerCount, buf);
            for (auto & p : msg.players) {
                appendRawBytes(p.clientId, buf);
                appendRawBytes(p.color, buf);
                appendRawBytes(p.direction, buf);
                appendRawBytes(p.score, buf);
                appendRawBytes(p.username, buf);
                segmentCount = static_cast<uint32_t>(p.segments.size());
                appendRawBytes(segmentCount, buf);
                for (auto & s : p.segments) {
                    appendRawBytes(s.first, buf);
                    appendRawBytes(s.second, buf);
                }
            }

            return buf;
        } else {
            throw std::runtime_error("Invalid MessageType");
        }
    }

    inline Bytes serialise(const MessageVariant & msg) {
        return std::visit([](const auto & m) -> Bytes {return serialise(m);}, msg);
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
        case MessageType::GAME_STATE: {
            GameState msg;
            msg.hdr = hdr;
            readRawBytes(raw, msg.highScore);
            readRawBytes(raw, msg.highScoreUsername);

            uint32_t foodCount;
            readRawBytes(raw, foodCount);
            msg.food.reserve(foodCount);
            for (uint32_t i = 0; i < foodCount; i++) {
                GameState::Food f;
                readRawBytes(raw, f.color);
                readRawBytes(raw, f.icon);
                readRawBytes(raw, f.x);
                readRawBytes(raw, f.y);
                msg.food.push_back(f);
            }

            uint32_t speedBoostCount;
            readRawBytes(raw, speedBoostCount);
            msg.speedBoosts.reserve(speedBoostCount);
            for (uint32_t i = 0; i < speedBoostCount; i++) {
                GameState::SpeedBoost sb;
                readRawBytes(raw, sb.color);
                readRawBytes(raw, sb.icon);
                readRawBytes(raw, sb.x);
                readRawBytes(raw, sb.y);
                msg.speedBoosts.push_back(sb);
            }

            uint32_t playerCount;
            uint32_t segmentCount;
            readRawBytes(raw, playerCount);
            msg.players.reserve(playerCount);
            for (uint32_t i = 0; i < playerCount; i++) {
                GameState::Player p;
                readRawBytes(raw, p.clientId);
                readRawBytes(raw, p.color);
                readRawBytes(raw, p.direction);
                readRawBytes(raw, p.score);
                readRawBytes(raw, p.username);
                readRawBytes(raw, segmentCount);
                p.segments.reserve(segmentCount);
                for (uint32_t j = 0; j < segmentCount; j++) {
                    GameState::Player::Segment s;
                    readRawBytes(raw, s.first);
                    readRawBytes(raw, s.second);
                    p.segments.push_back(s);
                }
                msg.players.push_back(std::move(p));
            }
            return msg;
        }
        default:
            throw std::runtime_error("Invalid MessageType");
        }
    }

    inline MessageVariant deserialise(const Bytes & buf, const int clientId) {
        MessageVariant msg {deserialise(buf)};
        header(msg).clientId = clientId;
        return msg;
    }
}; // namespace protocol
