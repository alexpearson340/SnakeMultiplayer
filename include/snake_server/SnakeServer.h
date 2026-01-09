#pragma once

#include <unordered_map>
#include <unordered_set>
#include <random>
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
    void moveSnakes();
    void updateOccupiedCells(const int);
    void checkCollisions();
    void destroyPlayers(std::vector<int> &);
    void broadcastGameState();
    std::string buildGameStatePayload();

    int width;
    int height;
    bool running;
    int gameTickMs;
    std::mt19937 gen;

    NetworkServer network;
    std::unordered_map<int, Player> clientIdToPlayerMap;
    std::unordered_map<std::pair<int, int>, std::unordered_set<int>, PairHash> occupiedCellsBodies;
    std::unordered_map<std::pair<int, int>, std::unordered_set<int>, PairHash> occupiedCellsHeads;
};

