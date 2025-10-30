#include "snake_server/SnakeServer.h"

SnakeServer::SnakeServer(int width, int height)
    : Engine(width, height)
    , network() {
}

void SnakeServer::handleInput() {
    network.pollMessages();
    int ch = getch();
    if (ch != ERR) {
        if (ch == 'q' || ch == 'Q') {
            running = false;
        }
        else if (ch == KEY_UP) {
        }
        else if (ch == KEY_DOWN) {
        }
        else if (ch == KEY_LEFT) {
        }
        else if (ch == KEY_RIGHT) {
        }
    }
    flushinp();  // Flush any remaining input
}

void SnakeServer::create() {
}

void SnakeServer::update() {
}

void SnakeServer::render() {
}

void SnakeServer::cleanup() {
}
