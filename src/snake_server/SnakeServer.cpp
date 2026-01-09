#include <iostream>
#include <stdexcept>
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
            case MessageType::CLIENT_JOIN:
                handleClientJoin(msg);
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

void SnakeServer::handleClientConnect(const ProtocolMessage & msg) {
    std::cout << "Adding new player " << msg.message << std::endl;
    clientIdToPlayerMap.emplace(msg.clientId, Player { PlayerNode(width / 2, height / 2), '^', msg.message});
}

void SnakeServer::handleClientJoin(const ProtocolMessage & msg) {
    std::cout << "New client joined: " << msg.message << std::endl;
    // todo player construction and placement
    clientIdToPlayerMap.emplace(
        msg.clientId,
        Player {
            PlayerNode{width / 2, height / 2},
            '^',
            msg.message,
            1
        }
    );

    // send a SERVER_WELCOME message back to the client, confirming that they are playing
    ProtocolMessage serverWelcomeMessage {
        MessageType::SERVER_WELCOME,
        msg.clientId,
        ""
    };
    network.sendToClient(msg.clientId, protocol::toString(serverWelcomeMessage));
}

void SnakeServer::handleClientDisconnect(const ProtocolMessage & msg) {
    std::cout << "Deleting player " << msg.message << std::endl;
    clientIdToPlayerMap.erase(msg.clientId);
}

void SnakeServer::handleClientInput(const ProtocolMessage & msg) {
    Player & player {clientIdToPlayerMap.at(msg.clientId)};
    if (msg.message == SnakeConstants::KEY_QUIT) {
        running = false;
    }
    else if (msg.message == SnakeConstants::KEY_UP) {
        if (player.direction != 'v') {
            player.direction = '^';
        }
    }
    else if (msg.message == SnakeConstants::KEY_DOWN) {
        if (player.direction != '^') {
            player.direction = 'v';
        }
    }
    else if (msg.message == SnakeConstants::KEY_LEFT) {
        if (player.direction != '>') {
            player.direction = '<';
        }
    }
    else if (msg.message == SnakeConstants::KEY_RIGHT) {
        if (player.direction != '<') {
            player.direction = '>';
        }
    }
    else {
        std::cout << "Unexpected receive from clientId(" << msg.clientId << "): " << msg.message << std::endl;
    }
}

void SnakeServer::create() {
}

void SnakeServer::update() {
    network.broadcast(buildGameStatePayload());
}

std::string SnakeServer::buildGameStatePayload() {
    json gameState;

    // players
    gameState["players"] = json::array();
    for (auto & [clientId, player] : clientIdToPlayerMap) {
        json playerJson;
        playerJson["client_id"] = clientId;
        playerJson["direction"] = std::string(1, player.direction);
        playerJson["name"] = player.name;
        playerJson["score"] = player.score;
        playerJson["segments"] = json::array();

        // get the x and y coordinates of every body segment
        std::vector<std::pair<int, int>> segments {};
        player.head.getAllSegmentCoordinates(segments);
        for (auto segment : segments) {
            playerJson["segments"].push_back({segment.first, segment.second});
        }
        gameState["players"].push_back(playerJson);
    }

    // food
    ProtocolMessage msg {MessageType::GAME_STATE, -1, gameState.dump()};
    return protocol::toString(msg);
}

void SnakeServer::render() {
}

void SnakeServer::cleanup() {
}
