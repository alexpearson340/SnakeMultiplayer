#include "engine/Engine.h"

Engine::Engine(int width, int height)
    : width(width)
    , height(height)
    , running(true)
    , score(0)
    , gen(std::random_device()()) {
    create();
}

Engine::~Engine() {
    cleanup();
}

void Engine::initCurses() {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);
}

void Engine::cleanupCurses() {
    endwin();
}

void Engine::handleInput() {}

void Engine::create() {}

void Engine::update() {}

void Engine::render() {}

void Engine::cleanup() {}

void Engine::run() {
    while (running) {
        handleInput();
        update();
        render();
        napms(napMs);
    }
}
