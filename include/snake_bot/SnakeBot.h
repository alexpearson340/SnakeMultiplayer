#pragma once

#include <random>
#include "common/Timer.h"
#include "snake_client/GameState.h"
#include "snake_client/NetworkClient.h"
#include "snake_bot/Pathfinder.h"

class SnakeBot {
public:
    SnakeBot(const int, const int);
    void run();

private:
    void createBot();
    void joinGame();
    void receiveUpdates();
    void handleServerWelcome(const ProtocolMessage & msg);
    void handleGameStateMessage(const ProtocolMessage & msg);
    void buildArenaMap();
    void sendInput();
    const char calculateRandomMove(const int);
    const char calculatePathingMove(const int) const;

    int width;
    int height;
    bool awaitingJoin;
    bool isAlive;
    int clientId;
    Timer timer;
    std::mt19937 gen;
    NetworkClient network;
    client::GameState gameState;
    Pathfinder pathfinder;
};