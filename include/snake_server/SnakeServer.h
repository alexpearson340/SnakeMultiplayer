#pragma once

#include "common/Hash.h"
#include "common/MessageLogReader.h"
#include "common/MessageLogWriter.h"
#include "common/Timer.h"
#include "snake_server/NetworkServer.h"
#include "snake_server/Player.h"
#include "snake_server/ServerConfig.h"
#include <chrono>
#include <random>
#include <unordered_map>
#include <unordered_set>

class SnakeServer {
public:
    SnakeServer(const ServerConfig &, std::optional<MessageLogReader> &&);
    void run();

private:
    void recordServerConfig();
    bool isInReplay() const;
    std::optional<std::vector<protocol::MessageVariant>> pollMessages();
    void handleClientJoin(const protocol::ClientJoin &);
    void handleClientDisconnect(const protocol::ClientDisconnect &);
    void handleClientInput(const protocol::ClientInput &);
    void createNewPlayer(const protocol::ClientJoin &);
    bool updateSnakes();
    void moveSnake(const int);
    void updateOccupiedCells(const int);
    void checkCollisions();
    void destroyPlayers(std::vector<int> &);
    void feedPlayer(std::pair<int, int> &, const int);
    void boostPlayer(std::pair<int, int> &, const int);
    void replaceFood();
    void placeFood();
    void placeFood(const int, const int, const Color color = Color::WHITE);
    void placeSpeedBoost();
    void broadcastGameState();
    protocol::GameState buildGameState();
    void logEngineBenchmark(const std::chrono::time_point<std::chrono::steady_clock> &, const int64_t &);

    template <typename T>
    T stamped(T msg) {
        stampMessage(msg);
        return msg;
    }
    
    template <typename T>
    void stampMessage(T & msg) {
        if constexpr (std::is_same_v<T, protocol::MessageVariant>) {
            protocol::Header & hdr {protocol::header(msg)};
            hdr.sequence = currentSequence++;
            hdr.transactTime = timer.currentTickAsNanos();
        }
        else {
            msg.hdr.sequence = currentSequence++;
            msg.hdr.transactTime = timer.currentTickAsNanos();
        }
    }

    int width;
    int height;
    int64_t currentSequence;
    std::chrono::milliseconds movementFrequencyMs;
    std::chrono::milliseconds boostedMovementFrequencyMs;
    std::chrono::milliseconds boostDurationMs;
    Timer timer;
    std::uint32_t seed;
    std::mt19937 gen;
    MessageLogWriter msgLogWriter;
    std::pair<std::string, int> serverHighScore;

    std::optional<MessageLogReader> replayFile;
    NetworkServer network;
    std::unordered_map<int, Player> clientIdToPlayerMap;
    std::unordered_map<std::pair<int, int>, std::unordered_set<int>, PairHash> occupiedCellsBodies;
    std::unordered_map<std::pair<int, int>, std::unordered_set<int>, PairHash> occupiedCellsHeads;
    std::unordered_map<std::pair<int, int>, Food, PairHash> foodMap;
    std::unordered_map<std::pair<int, int>, SpeedBoost, PairHash> speedBoostMap;
};