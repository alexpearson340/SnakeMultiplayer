#include "engine/Engine.h"

class SnakeClient : public Engine {
public:
    SnakeClient(int width, int height);

    void handleInput();
    void create();
    void update();
    void render();
    void cleanup();
};
