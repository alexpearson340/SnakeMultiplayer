#include <iostream>
#include "snake_server/Constants.h"
#include "engine/Json.h"
#include "snake_server/SnakeServer.h"

SnakeServer::SnakeServer(int width, int height)
    : Engine(width, height)
    , network {8170}
    , clientIdToPlayerMap {} {
}

void SnakeServer::handleInput() {
    std::vector<ProtocolMessage> messages { network.pollMessages() };
    for (auto msg : messages) {
        switch (msg.messageType) {
            case MessageType::CLIENT_CONNECT:
                handleAcceptNewClient(msg);
                break;
            case MessageType::CLIENT_DISCONNECT:
                handleClientDisconnect(msg);
                break;
            case MessageType::CLIENT_INPUT:
                handleClientInput(msg);
                break;
            default:
                throw std::runtime_error("Invalid MessageType");
        }
    }
}

void SnakeServer::handleAcceptNewClient(const ProtocolMessage & msg) {
    std::cout << "Adding new player " << msg.message << std::endl;
    clientIdToPlayerMap.emplace(msg.clientId, Player { PlayerNode(width / 2, height / 2), '^', msg.message});
}

void SnakeServer::handleClientDisconnect(const ProtocolMessage & msg) {
    std::cout << "Deleting player " << msg.message << std::endl;
    clientIdToPlayerMap.erase(msg.clientId);
}

void SnakeServer::handleClientInput(const ProtocolMessage & msg) {
    if (msg.message == SnakeConstants::KEY_QUIT) {
        running = false;
    }
    else if (msg.message == SnakeConstants::KEY_UP) {
        std::cout << "KEY_UP" << std::endl;
        if (clientIdToPlayerMap.at(msg.clientId).direction != 'v') {
            clientIdToPlayerMap.at(msg.clientId).direction = '^';
        }
    }
    else if (msg.message == SnakeConstants::KEY_DOWN) {
        std::cout << "KEY_DOWN" << std::endl;
        if (clientIdToPlayerMap.at(msg.clientId).direction != '^') {
            clientIdToPlayerMap.at(msg.clientId).direction = 'v';
        }
    }
    else if (msg.message == SnakeConstants::KEY_LEFT) {
        std::cout << "KEY_LEFT" << std::endl;
        if (clientIdToPlayerMap.at(msg.clientId).direction != '>') {
            clientIdToPlayerMap.at(msg.clientId).direction = '<';
        }
    }
    else if (msg.message == SnakeConstants::KEY_RIGHT) {
        std::cout << "KEY_RIGHT" << std::endl;
        if (clientIdToPlayerMap.at(msg.clientId).direction != '<') {
            clientIdToPlayerMap.at(msg.clientId).direction = '>';
        }
    }
    else {
        std::cout << "Unexpected receive from clientId(" << msg.clientId << "): " << msg.message << std::endl;
    }
}

void SnakeServer::create() {
}

void SnakeServer::update() {
    network.broadcast(buildGameStatePayload() + '\n');
}

std::string SnakeServer::buildGameStatePayload() {
    json msg;
    msg["players"] = json::array();
    msg["food"] = json::array();
    return msg.dump();
}

void SnakeServer::render() {
}

void SnakeServer::cleanup() {
}
