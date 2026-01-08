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
    std::vector<ClientMessage> messages { network.pollMessages() };
    for (auto msg : messages) {
        switch (msg.messageType) {
            case ClientMessageType::ACCEPT_NEW_CLIENT:
                handleAcceptNewClient(msg);
                break;
            case ClientMessageType::CLIENT_DISCONNECT:
                handleClientDisconnect(msg);
                break;
            case ClientMessageType::CLIENT_INPUT:
                handleClientInput(msg);
                break;
            default:
                throw std::runtime_error("Invalid ClientMessageType");
        }
    }
}

void SnakeServer::handleAcceptNewClient(const ClientMessage & msg) {
    std::cout << "Adding new player " << msg.data << std::endl;
    clientIdToPlayerMap.emplace(msg.clientId, Player { PlayerNode(width / 2, height / 2), '^', msg.data});
}

void SnakeServer::handleClientDisconnect(const ClientMessage & msg) {
    std::cout << "Deleting player " << msg.data << std::endl;
    clientIdToPlayerMap.erase(msg.clientId);
}

void SnakeServer::handleClientInput(const ClientMessage & msg) {
    if (msg.data == SnakeConstants::KEY_UP) {
        if (clientIdToPlayerMap.at(msg.clientId).direction != 'v') {
            clientIdToPlayerMap.at(msg.clientId).direction = '^';
        }
    }
    else if (msg.data == SnakeConstants::KEY_DOWN) {
        if (clientIdToPlayerMap.at(msg.clientId).direction != '^') {
            clientIdToPlayerMap.at(msg.clientId).direction = 'v';
        }
    }
    else if (msg.data == SnakeConstants::KEY_LEFT) {
        if (clientIdToPlayerMap.at(msg.clientId).direction != '>') {
            clientIdToPlayerMap.at(msg.clientId).direction = '<';
        }
    }
    else if (msg.data == SnakeConstants::KEY_RIGHT) {
        if (clientIdToPlayerMap.at(msg.clientId).direction != '<') {
            clientIdToPlayerMap.at(msg.clientId).direction = '>';
        }
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
