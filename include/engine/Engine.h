#pragma once

#include <random>

class Engine {
public:
    Engine(int width, int height);
    virtual ~Engine();

    void run();

protected:
    virtual void handleInput();
    virtual void create();
    virtual void update();
    virtual void render();
    virtual void cleanup();

    int napMs [500];
    int width;
    int height;
    bool running;
    int lastKey;
    int score;

    // random
    std::mt19937 gen;
};
