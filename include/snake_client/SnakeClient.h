#pragma once

#include "engine/Engine.h"

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
};
