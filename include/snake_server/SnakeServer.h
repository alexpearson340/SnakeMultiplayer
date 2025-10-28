#include "engine/Engine.h"

class SnakeServer : public Engine {
public:
    SnakeServer(int width, int height);

    void handleInput();
    void create();
    void update();
    void render();
};

