#include <unordered_map>
#include "engine/Engine.h"
#include "engine/NetworkLayer.h"
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
    NetworkLayer network;
    std::unordered_map<int, Player> clientIdToPlayerMap;
};

