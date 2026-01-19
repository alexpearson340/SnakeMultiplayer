#pragma once

#include <random>
#include "snake_client/GameState.h"
#include "snake_client/NetworkClient.h"

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

    int width;
    int height;
    bool awaitingJoin;
    bool isAlive;
    int clientId;
    std::mt19937 gen;
    NetworkClient network;
    client::GameState gameState;
};