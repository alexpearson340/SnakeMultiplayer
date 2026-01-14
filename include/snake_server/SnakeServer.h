#pragma once

#include <chrono>
#include <unordered_map>
#include <random>
#include <unordered_set>
#include "common/Hash.h"
#include "snake_server/NetworkServer.h"
#include "snake_server/Player.h"

class SnakeServer {
public:
    SnakeServer(int width, int height);

    void run();

private:
    void handleClientJoin(const ProtocolMessage &);
    void handleClientConnect(const ProtocolMessage &);
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
    void logGameState();
    void logOccupiedCells();
    
    int width;
    int height;
    bool running;
    std::chrono::milliseconds movementFrequencyMs;
    std::chrono::milliseconds boostedMovementFrequencyMs;
    std::chrono::milliseconds boostDurationMs;
    std::chrono::time_point<std::chrono::steady_clock> currentGameTick;
    std::mt19937 gen;
    std::pair <std::string, int> serverHighScore;

    NetworkServer network;
    std::unordered_map<int, Player> clientIdToPlayerMap;
    std::unordered_map<std::pair<int, int>, std::unordered_set<int>, PairHash> occupiedCellsBodies;
    std::unordered_map<std::pair<int, int>, std::unordered_set<int>, PairHash> occupiedCellsHeads;
    std::unordered_map<std::pair<int, int>, Food, PairHash> foodMap;
    std::unordered_map<std::pair<int, int>, SpeedBoost, PairHash> speedBoostMap;
};

