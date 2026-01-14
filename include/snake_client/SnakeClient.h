#pragma once

#include "snake_client/GameState.h"
#include "snake_client/NetworkClient.h"

class SnakeClient {
public:
    SnakeClient(int width, int height);
    ~SnakeClient();
    void run();

private:
    void joinGame();
    void handleInput();
    void sendPlayerInput();
    void receiveUpdates();
    void handleServerWelcome(const ProtocolMessage &);
    void handleGameStateMessage(const ProtocolMessage &);
    void render();
    void renderArena();
    void renderPlayers();
    void renderObjects();
    void renderScore();
    void renderCharToScreen(const int, const int, const char &, const int color = 1);

    void initNcurses();
    void cleanupNcurses();

    int width;
    int height;
    bool running;
    bool playing;
    int score;

    NetworkClient network;
    int clientId;
    char playerInput;
    client::GameState gameState;
};
