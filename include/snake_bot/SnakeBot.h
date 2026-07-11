#pragma once

#include "common/Timer.h"
#include "snake_bot/Pathfinder.h"
#include "snake_client/GameState.h"
#include "snake_client/NetworkClient.h"
#include <random>

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
    char calculateRandomMove();
    char calculatePathingMove() const;

    bool awaitingJoin;
    bool gameStateHasChanged;
    int clientId;
    Timer timer;
    std::mt19937 gen;
    NetworkClient network;
    client::GameState gameState;
    Pathfinder pathfinder;
};