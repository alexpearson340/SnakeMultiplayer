#include <iostream>
#include <stdexcept>
#include <chrono>
#include "common/Constants.h"
#include "common/Json.h"
#include "snake_server/SnakeServer.h"

SnakeServer::SnakeServer(int width, int height)
    : width {width}
    , height {height}
    , running {true}
    , movementFrequencyMs(std::chrono::milliseconds(MOVEMENT_FREQUENCY_MS))
    , currentGameTick(std::chrono::steady_clock::now())
    , gen {std::random_device{}()}
    , serverHighScore {}
    , network {SERVER_PORT}
    , clientIdToPlayerMap {}
    , occupiedCellsBodies {}
    , occupiedCellsHeads {}
    , foodMap {} {
}

void SnakeServer::run() {
    while (true) {
        currentGameTick = std::chrono::steady_clock::now();
        replaceFood();
        std::vector<ProtocolMessage> messages { network.pollMessages() };
        bool stateChanged = false;

        for (auto msg : messages) {
            switch (msg.messageType) {
                case MessageType::CLIENT_JOIN:
                    handleClientJoin(msg);
                    stateChanged = true;
                    break;
                case MessageType::CLIENT_INPUT:
                    handleClientInput(msg);
                    stateChanged = true;
                    break;
                case MessageType::CLIENT_DISCONNECT:
                    handleClientDisconnect(msg);
                    stateChanged = true;
                    break;
                default:
                    throw std::runtime_error("Invalid MessageType");
            }
        }

        if (updateSnakes()) {
            checkCollisions();
            stateChanged = true;
        }

        if (stateChanged) {
            broadcastGameState();
        }
    }
}

void SnakeServer::handleClientConnect(const ProtocolMessage & msg) {
    std::cout << "Adding new player " << msg.message << std::endl;
    createNewPlayer(msg);
}

void SnakeServer::handleClientJoin(const ProtocolMessage & msg) {
    createNewPlayer(msg);

    // send a SERVER_WELCOME message back to the client, confirming that they are playing
    ProtocolMessage serverWelcomeMessage {
        MessageType::SERVER_WELCOME,
        msg.clientId,
        ""
    };
    network.sendToClient(msg.clientId, protocol::toString(serverWelcomeMessage));
}

void SnakeServer::createNewPlayer(const ProtocolMessage & msg) {
    // todo player placement
    clientIdToPlayerMap.emplace(
        msg.clientId,
        Player {
            PlayerNode{width / 2, height / 2},
            '^',
            '^',
            msg.message,
            1,
            static_cast<Color>((msg.clientId % 6) + 2),
            movementFrequencyMs,
            currentGameTick + movementFrequencyMs
        }
    );
}

void SnakeServer::handleClientDisconnect(const ProtocolMessage & msg) {
    std::cout << "Deleting player " << msg.message << std::endl;
    clientIdToPlayerMap.erase(msg.clientId);
}

void SnakeServer::handleClientInput(const ProtocolMessage & msg) {
    if (!clientIdToPlayerMap.contains(msg.clientId)) {
        std::cout << "Ignoring input from unknown clientId: " << msg.clientId << std::endl;
        return;
    }

    Player & player {clientIdToPlayerMap.at(msg.clientId)};
    if (msg.message == SnakeConstants::KEY_QUIT) {
        running = false;
    }
    else if (msg.message == SnakeConstants::PLAYER_KEY_UP) {
        if (player.direction != 'v') {
            player.nextDirection = '^';
        }
    }
    else if (msg.message == SnakeConstants::PLAYER_KEY_DOWN) {
        if (player.direction != '^') {
            player.nextDirection = 'v';
        }
    }
    else if (msg.message == SnakeConstants::PLAYER_KEY_LEFT) {
        if (player.direction != '>') {
            player.nextDirection = '<';
        }
    }
    else if (msg.message == SnakeConstants::PLAYER_KEY_RIGHT) {
        if (player.direction != '<') {
            player.nextDirection = '>';
        }
    }
    else {
        std::cout << "Unexpected receive from clientId(" << msg.clientId << "): " << msg.message << std::endl;
    }
}

bool SnakeServer::updateSnakes() {
    bool snakeUpdates {false};

    // todo maybe do a differential update to occupiedCells if we need better performance
    occupiedCellsBodies.clear();
    occupiedCellsHeads.clear();
    for (auto & [clientId, player] : clientIdToPlayerMap) {
        if (currentGameTick >= player.nextMoveTime) {
            moveSnake(clientId);
            snakeUpdates = true;
        }
        updateOccupiedCells(clientId);
    }
    return snakeUpdates;
}

void SnakeServer::moveSnake(const int clientId) {
    Player & player {clientIdToPlayerMap.at(clientId)};
    player.direction = player.nextDirection;
    switch (player.direction) {
        case '^':
            player.head.move(0, -1);
            break;
        case 'v':
            player.head.move(0, 1);
            break;
        case '<':
            player.head.move(-1, 0);
            break;
        case '>':
            player.head.move(1, 0);
            break;
        default:
            throw std::runtime_error("Invalid direction: " + std::string(1, player.direction));
    }
    player.nextMoveTime = currentGameTick + player.movementFrequencyMs;
}

void SnakeServer::updateOccupiedCells(const int clientId) {
    std::vector<std::pair<int, int>> segments {};
    clientIdToPlayerMap.at(clientId).head.getSegments(segments);
    occupiedCellsHeads[segments[0]].insert(clientId);
    for (auto it = segments.begin() + 1; it < segments.end(); it++) {
        occupiedCellsBodies[*it].insert(clientId);
    }
}

void SnakeServer::checkCollisions() {
    std::vector<int> clientIdsToDestroy;
    for (auto & [clientId, player] : clientIdToPlayerMap) {
        std::pair<int, int> playerHead {player.head.x(), player.head.y()};

        // collision with arena boundary
        if (player.head.y() <= 0) {
            clientIdsToDestroy.push_back(clientId);
        }
        else if (player.head.y() >= height + 1) {
            clientIdsToDestroy.push_back(clientId);
        }
        else if (player.head.x() <= 0) {
            clientIdsToDestroy.push_back(clientId);
        }
        else if (player.head.x() >= width + 1) {
            clientIdsToDestroy.push_back(clientId);
        }
        // collision with a snake body segment
        else if (occupiedCellsBodies.contains(playerHead)) {
            clientIdsToDestroy.push_back(clientId);
        }
        // collision with a different snake's head
        else if (occupiedCellsHeads.contains(playerHead) && occupiedCellsHeads.at(playerHead).size() > 1) {
            clientIdsToDestroy.push_back(clientId);
        }

        // get food
        else if (foodMap.contains(playerHead)) {
            feedPlayer(playerHead, clientId);

            // track all time score
            if (player.score > serverHighScore.second) {
                serverHighScore = {player.name, player.score};
                std::cout << "New server high score, " << player.name << ": " << player.score << std::endl;
            }
        }
    }
    if (!clientIdsToDestroy.empty()) {
        destroyPlayers(clientIdsToDestroy);
    }
}

void SnakeServer::destroyPlayers(std::vector<int> & clientIds) {
    std::uniform_int_distribution<> dist(1, 3);
    for (auto & id : clientIds) {
        Player & player {clientIdToPlayerMap.at(id)};
        std::vector<std::pair<int, int>> segments {};
        player.head.getSegments(segments);
        for (auto it = segments.begin() + 1; it < segments.end(); it++) {
            if (dist(gen) == 1) {
                placeFood(it->first, it->second, player.color);
            }
        }

        // delete the player
        clientIdToPlayerMap.erase(id);
    }
}

void SnakeServer::feedPlayer(std::pair<int, int> & playerCell, const int clientId) {
    clientIdToPlayerMap.at(clientId).head.grow();
    clientIdToPlayerMap.at(clientId).score++;
    foodMap.erase(playerCell);
}

void SnakeServer::replaceFood() {
    if (foodMap.size() < MIN_FOOD_IN_ARENA) {
        placeFood();
    }
}

void SnakeServer::placeFood() {
    std::uniform_int_distribution<> distX(1, width - 1);
    std::uniform_int_distribution<> distY(1, height - 1);
    placeFood(distX(gen), distY(gen));
}

void SnakeServer::placeFood(const int x, const int y, const Color color) {
    std::cout << "Placing food at (" << x << ", " << y << ")" << std::endl;
    foodMap[std::pair<int, int> {x, y}] = Food {x, y, '@', color};
}

void SnakeServer::broadcastGameState() {
    network.broadcast(buildGameStatePayload());
}

std::string SnakeServer::buildGameStatePayload() {
    json gameState;
    gameState["server_high_score"] = {serverHighScore.first, serverHighScore.second};

    // players
    gameState["players"] = json::array();
    for (auto & [clientId, player] : clientIdToPlayerMap) {
        json playerJson;
        playerJson["client_id"] = clientId;
        playerJson["direction"] = std::string(1, player.direction);
        playerJson["name"] = player.name;
        playerJson["score"] = player.score;
        playerJson["color"] = player.color;
        playerJson["segments"] = json::array();

        // get the x and y coordinates of every body segment
        std::vector<std::pair<int, int>> segments {};
        player.head.getSegments(segments);
        for (auto segment : segments) {
            playerJson["segments"].push_back({segment.first, segment.second});
        }
        gameState["players"].push_back(playerJson);
    }

    // food
    gameState["food"] = json::array();
    for (auto & [coords, food] : foodMap) {
        json foodJson;
        foodJson["x"] = food.x;
        foodJson["y"] = food.y;
        foodJson["icon"] = std::string(1, food.icon);
        foodJson["color"] = food.color;
        gameState["food"].push_back(foodJson);
    }

    ProtocolMessage msg {MessageType::GAME_STATE, -1, gameState.dump()};
    return protocol::toString(msg);
}
