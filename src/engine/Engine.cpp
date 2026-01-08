#include <chrono>
#include <thread>
#include "engine/Engine.h"

Engine::Engine(int width, int height)
    : width(width)
    , height(height)
    , running(true)
    , score(0)
    , gen(std::random_device()()) {
}

Engine::~Engine() {}

void Engine::handleInput() {}

void Engine::create() {}

void Engine::update() {}

void Engine::render() {}

void Engine::cleanup() {}

void Engine::run() {
    create();
    auto lastFrame = std::chrono::steady_clock::now();
    while (running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrame);

        if (elapsed.count() >= napMs) {
            handleInput();
            update();
            render();
            lastFrame = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
