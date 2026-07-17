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
    void stampMessage(ProtocolMessage &);
    ProtocolMessage stamped(ProtocolMessage msg);
    bool isInReplay() const;
    std::optional<std::vector<ProtocolMessage>> pollMessages();
    void handleClientJoin(const ProtocolMessage &);
    void handleClientDisconnect(const ProtocolMessage &);
    void handleClientInput(const ProtocolMessage &);
    void createNewPlayer(const ProtocolMessage &);
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
    std::string buildGameStatePayload();
    void logEngineBenchmark(const std::chrono::time_point<std::chrono::steady_clock> &, const int64_t &);

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