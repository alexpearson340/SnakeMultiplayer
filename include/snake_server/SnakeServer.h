#pragma once

#include <unordered_map>
#include <random>
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
    void broadcastGameState();
    std::string buildGameStatePayload();

    int width;
    int height;
    bool running;
    int gameTickMs {500};
    std::mt19937 gen;

    NetworkServer network;
    std::unordered_map<int, Player> clientIdToPlayerMap;
};

