#pragma once

#include <unordered_map>
#include "engine/Engine.h"
#include "snake_server/NetworkServer.h"
#include "snake_server/Player.h"

class SnakeServer : public Engine {
public:
    SnakeServer(int width, int height);

    void handleInput();
    void create();
    void update();
    void render();
    void cleanup();

private:
    void handleAcceptNewClient(const ClientMessage &);
    void handleClientDisconnect(const ClientMessage &);
    void handleClientInput(const ClientMessage &);
    std::string buildGameStatePayload();

    NetworkServer network;
    std::unordered_map<int, Player> clientIdToPlayerMap;
};

