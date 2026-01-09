#pragma once

#include "engine/Engine.h"
#include "snake_client/GameState.h"
#include "snake_client/NetworkClient.h"

class SnakeClient : public Engine {
public:
    SnakeClient(int width, int height);
    ~SnakeClient();

    void handleInput() override;
    void create() override;
    void update() override;
    void render() override;
    void cleanup() override;

protected:
    void initNcurses();
    void cleanupNcurses();
    void sendPlayerInput();
    void receiveUpdates();
    void handleServerWelcome(const ProtocolMessage &);
    void handleGameStateMessage(const ProtocolMessage &);
    void renderArena();
    void renderPlayers();
    void renderFood();
    void renderScore();

    NetworkClient network;
    int clientId;
    char playerInput;
    client::GameState gameState;
};
