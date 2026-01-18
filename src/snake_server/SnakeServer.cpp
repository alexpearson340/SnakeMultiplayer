#include <iostream>
#include <stdexcept>
#include <chrono>
#include "common/Constants.h"
#include "common/Json.h"
#include "snake_server/SnakeServer.h"

SnakeServer::SnakeServer(int width, int height)
    : width {width}
    , height {height}
    , movementFrequencyMs(std::chrono::milliseconds(MOVEMENT_FREQUENCY_MS))
    , boostedMovementFrequencyMs(std::chrono::milliseconds(BOOSTED_MOVEMENT_FREQUENCY_MS))
    , boostDurationMs(std::chrono::milliseconds(SPEED_BOOST_DURATION_MS))
    , currentGameTick(std::chrono::steady_clock::now())
    , gen {std::random_device{}()}
    , serverHighScore {}
    , network {SERVER_PORT}
    , clientIdToPlayerMap {}
    , occupiedCellsBodies {}
    , occupiedCellsHeads {}
    , foodMap {}
    , speedBoostMap {} {
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
            placeSpeedBoost();
            stateChanged = true;
        }

        if (stateChanged) {
            broadcastGameState();
        }
    }
}

void SnakeServer::handleClientJoin(const ProtocolMessage & msg) {
    std::cout << "New client joined: " << msg.message << std::endl;
    createNewPlayer(msg);

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
    if (!clientIdToPlayerMap.contains(msg.clientId)) {
        std::cout << "Ignoring input from unknown clientId: " << msg.clientId << std::endl;
        return;
    }
    Player & player {clientIdToPlayerMap.at(msg.clientId)};

    if (msg.message == SnakeConstants::PLAYER_KEY_UP) {
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

void SnakeServer::createNewPlayer(const ProtocolMessage & msg) {
    std::uniform_int_distribution<> distX(1 + 5, width - 1 - 5);
    std::uniform_int_distribution<> distY(1 + 5, height - 1 - 5);
    clientIdToPlayerMap.emplace(
        msg.clientId,
        Player {
            PlayerNode {distX(gen), distY(gen)},
            '^',
            '^',
            msg.message,
            1,
            static_cast<Color>((msg.clientId % 5) + 2),
            movementFrequencyMs,
            currentGameTick + movementFrequencyMs,
            false,
            currentGameTick
        }
    );
}

bool SnakeServer::updateSnakes() {
    bool snakeUpdates {false};

    // todo maybe do a differential update to occupiedCells if we need better performance
    occupiedCellsBodies.clear();
    occupiedCellsHeads.clear();
    for (auto & [clientId, player] : clientIdToPlayerMap) {
        if (player.boosted && player.boostExpireTime <= currentGameTick) {
            player.boosted = false;
            player.movementFrequencyMs = movementFrequencyMs;
        }
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
            std::cout << "Destroying " << player.name << " due to upper boundary collision" << std::endl;
            clientIdsToDestroy.push_back(clientId);
        }
        else if (player.head.y() >= height + 1) {
            std::cout << "Destroying " << player.name << " due to lower boundary collision" << std::endl;
            clientIdsToDestroy.push_back(clientId);
        }
        else if (player.head.x() <= 0) {
            std::cout << "Destroying " << player.name << " due to left boundary collision" << std::endl;
            clientIdsToDestroy.push_back(clientId);
        }
        else if (player.head.x() >= width + 1) {
            std::cout << "Destroying " << player.name << " due to upper boundary collision" << std::endl;
            clientIdsToDestroy.push_back(clientId);
        }

        // collision with a snake body segment
        else if (occupiedCellsBodies.contains(playerHead)) {
            std::cout << "Destroying " << player.name << " due to snake body collision" << std::endl;
            clientIdsToDestroy.push_back(clientId);
        }

        // collision with a different snake's head
        else if (occupiedCellsHeads.contains(playerHead) && occupiedCellsHeads.at(playerHead).size() > 1) {
            std::cout << "Destroying " << player.name << " due to snake head collision" << std::endl;
            clientIdsToDestroy.push_back(clientId);
        }

        // get food
        else if (foodMap.contains(playerHead)) {
            std::cout << "Feeding player " << player.name << " at " << "(";
            std::cout << playerHead.first << ", " << playerHead.second << ")" << std::endl;
            feedPlayer(playerHead, clientId);
        }

        // get speed boost
        else if (speedBoostMap.contains(playerHead)) {
            std::cout << "Boosting player " << player.name << " at " << "(";
            std::cout << playerHead.first << ", " << playerHead.second << ")" << std::endl;
            boostPlayer(playerHead, clientId);
        }

        // track all time score
        if (player.score > serverHighScore.second) {
            serverHighScore = {player.name, player.score};
            std::cout << "New server high score, " << player.name << ": " << player.score << std::endl;
        }
    }
    if (!clientIdsToDestroy.empty()) {
        logGameState();
        logOccupiedCells();
        destroyPlayers(clientIdsToDestroy);
    }
}

void SnakeServer::destroyPlayers(std::vector<int> & clientIds) {
    std::uniform_int_distribution<> dist(1, FOOD_SPAWN_FROM_BODY_SEGMENT_PROBABILITY);
    for (auto & id : clientIds) {

        // chance to spawn food on player death for each body segment
        std::vector<std::pair<int, int>> segments {};
        Player & player {clientIdToPlayerMap.at(id)};
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

void SnakeServer::boostPlayer(std::pair<int, int> & playerCell, const int clientId) {
    clientIdToPlayerMap.at(clientId).boosted = true;
    clientIdToPlayerMap.at(clientId).movementFrequencyMs = boostedMovementFrequencyMs;
    clientIdToPlayerMap.at(clientId).boostExpireTime = currentGameTick + boostDurationMs;
    speedBoostMap.erase(playerCell);
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

void SnakeServer::placeSpeedBoost() {
    if (speedBoostMap.empty()) {
        std::uniform_int_distribution<> dist(1, SPEED_BOOST_PROBABILITY);
        if (dist(gen) == 1) {
            std::uniform_int_distribution<> distX(1, width - 1);
            std::uniform_int_distribution<> distY(1, height - 1);
            int x {distX(gen)};
            int y {distY(gen)};
            std::cout << "Placing speed boost at (" << x << ", " << y << ")";
            speedBoostMap[std::pair<int, int> {x, y}] = SpeedBoost {x, y, '*', Color::WHITE};
        }
    }
}

void SnakeServer::broadcastGameState() {
    network.broadcast(protocol::toString(ProtocolMessage {
        MessageType::GAME_STATE, -1, buildGameStatePayload()}));
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

    // speed boost
    gameState["speed_boosts"] = json::array();
    for (auto & [coords, speedBoost] : speedBoostMap) {
        json speedBoostJson;
        speedBoostJson["x"] = speedBoost.x;
        speedBoostJson["y"] = speedBoost.y;
        speedBoostJson["icon"] = std::string(1, speedBoost.icon);
        speedBoostJson["color"] = speedBoost.color;
        gameState["speed_boosts"].push_back(speedBoostJson);
    }

    return gameState.dump();
}

void SnakeServer::logGameState() {
    std::cout << "Game state: " << buildGameStatePayload() << std::endl;
}

void SnakeServer::logOccupiedCells() {
    std::cout << "Occupied cells (bodies):" << std::endl;
    for (auto & [cell, players] : occupiedCellsBodies) {
        std::cout << "(" << cell.first << ", " << cell.second << "): {";
        for (auto & player : players) {
            std::cout << player << " ";
        }
        std::cout << "}" << std::endl;
    }
    std::cout << "Occupied cells (heads):" << std::endl;
    for (auto & [cell, players] : occupiedCellsHeads) {
        std::cout << "(" << cell.first << ", " << cell.second << "): {";
        for (auto & player : players) {
            std::cout << player << " ";
        }
        std::cout << "}" << std::endl;
    }
}