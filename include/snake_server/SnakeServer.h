#include "engine/Engine.h"
#include "engine/NetworkLayer.h"

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
};

