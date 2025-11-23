#include <iostream>
#include "snake_server/Constants.h"
#include "snake_server/SnakeServer.h"

SnakeServer::SnakeServer(int width, int height)
    : Engine(width, height)
    , network {8170} 
    , clientIdToPlayerMap {} {
}

void SnakeServer::handleInput() {
    std::vector<ClientMessage> messages {network.pollMessages()}; 
    for (auto msg: messages) {
        if (msg.data == SnakeConstants::KEY_QUIT) {
            running = false;
        }
        else if (msg.data == SnakeConstants::KEY_UP) {
            std::cout << "KEY_UP" << std::endl;
        }
        else if (msg.data == SnakeConstants::KEY_DOWN) {
            std::cout << "KEY DOWN" << std::endl;
        }
        else if (msg.data == SnakeConstants::KEY_LEFT) {
            std::cout << "KEY_LEFT" << std::endl;
        }
        else if (msg.data == SnakeConstants::KEY_RIGHT) {
            std::cout << "KEY_RIGHT" << std::endl;
        }
        else {
            std::cout << "Unexpected receive from client (clientid" << msg.clientId << "): " << msg.data << std::endl;
        }
    }
} 

void SnakeServer::create() {
}

void SnakeServer::update() {
}

void SnakeServer::render() {
}

void SnakeServer::cleanup() {
}