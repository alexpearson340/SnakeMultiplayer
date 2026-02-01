#include <stdexcept>
#include <chrono>
#include "common/Log.h"
#include <string>
#include "common/Constants.h"
#include "common/Json.h"
#include "snake_server/SnakeServer.h"

SnakeServer::SnakeServer(int width, int height)
    : width {width}
    , height {height}
    , movementFrequencyMs(std::chrono::milliseconds(MOVEMENT_FREQUENCY_MS))
    , boostedMovementFrequencyMs(std::chrono::milliseconds(BOOSTED_MOVEMENT_FREQUENCY_MS))
    , boostDurationMs(std::chrono::milliseconds(SPEED_BOOST_DURATION_MS))
    , timer {}
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
        timer.tick();
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
    spdlog::info("Received client join request from " + msg.message);
    createNewPlayer(msg);

    // send a SERVER_WELCOME message back to the client, confirming that they are playing
    ProtocolMessage serverWelcomeMessage {
        MessageType::SERVER_WELCOME,
        msg.clientId,
        ""
    };
    network.sendToClient(msg.clientId, protocol::toString(serverWelcomeMessage));
    spdlog::info("Assigned clientId=" + std::to_string(msg.clientId) + " to new client " + msg.message);
    spdlog::info("Sent client welcome to " + msg.message);
}

void SnakeServer::handleClientDisconnect(const ProtocolMessage & msg) {
    spdlog::info("Deleting player " + msg.message);
    clientIdToPlayerMap.erase(msg.clientId);
}

void SnakeServer::handleClientInput(const ProtocolMessage & msg) {
    if (!clientIdToPlayerMap.contains(msg.clientId)) {
        spdlog::info("Ignoring input from unknown clientId: " + std::to_string(msg.clientId));
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
        spdlog::info("Unexpected receive from clientId(" + std::to_string(msg.clientId) + "): " + msg.message);
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
            timer.currentTick() + movementFrequencyMs,
            false,
            timer.currentTick()
        }
    );
}

bool SnakeServer::updateSnakes() {
    bool snakeUpdates {false};

    // todo maybe do a differential update to occupiedCells if we need better performance
    occupiedCellsBodies.clear();
    occupiedCellsHeads.clear();
    for (auto & [clientId, player] : clientIdToPlayerMap) {
        if (player.boosted && player.boostExpireTime <= timer.currentTick()) {
            player.boosted = false;
            player.movementFrequencyMs = movementFrequencyMs;
        }
        if (timer.currentTick() >= player.nextMoveTime) {
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
    player.nextMoveTime = timer.currentTick() + player.movementFrequencyMs;
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
            spdlog::info("Destroying " + player.name + " due to upper boundary collision");
            clientIdsToDestroy.push_back(clientId);
        }
        else if (player.head.y() >= height + 1) {
            spdlog::info("Destroying " + player.name + " due to lower boundary collision");
            clientIdsToDestroy.push_back(clientId);
        }
        else if (player.head.x() <= 0) {
            spdlog::info("Destroying " + player.name + " due to left boundary collision");
            clientIdsToDestroy.push_back(clientId);
        }
        else if (player.head.x() >= width + 1) {
            spdlog::info("Destroying " + player.name + " due to upper boundary collision");
            clientIdsToDestroy.push_back(clientId);
        }

        // collision with another snake's body
        else if (occupiedCellsBodies.contains(playerHead)) {
            spdlog::info("Destroying " + player.name + " due to snake body collision");
            clientIdsToDestroy.push_back(clientId);
        }

        // head-on-head snake collision - whoever arrived into the cell first survives
        else if (occupiedCellsHeads.at(playerHead).size() > 1) {
            int firstPlayerinCell {*std::min_element(
                occupiedCellsHeads.at(playerHead).begin(),
                occupiedCellsHeads.at(playerHead).end(),
                // Checking who was first in the cell based on nextMoveTime is slightly imperfect.
                // A client with a faster move speed can arrive later and still have a lower nextMoveTime.
                // This is probably good enough though
                [this] (const int a, const int b) {return clientIdToPlayerMap.at(a).nextMoveTime < clientIdToPlayerMap.at(b).nextMoveTime;}
            )};
            if (clientId != firstPlayerinCell) {
                spdlog::info("Destroying " + player.name + " due to snake head collision");
                clientIdsToDestroy.push_back(clientId);
            }
        }

        // get food
        else if (foodMap.contains(playerHead)) {
            spdlog::debug("Feeding player " + player.name + " at " + "(" + \
                std::to_string(playerHead.first) + ", " + std::to_string(playerHead.second) + ")");
            feedPlayer(playerHead, clientId);
        }

        // get speed boost
        else if (speedBoostMap.contains(playerHead)) {
            spdlog::debug("Boosting player " + player.name + " at " + "(" + \
                std::to_string(playerHead.first) + ", " + std::to_string(playerHead.second) + ")");
            boostPlayer(playerHead, clientId);
        }

        // track all time score
        if (player.score > serverHighScore.second) {
            serverHighScore = {player.name, player.score};
            spdlog::debug("New server high score, " + player.name + ": " + std::to_string(player.score));
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
    clientIdToPlayerMap.at(clientId).boostExpireTime = timer.currentTick() + boostDurationMs;
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
    spdlog::debug("Placing food at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
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
            spdlog::debug("Placing speed boost at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
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
    spdlog::debug("Game state: " + buildGameStatePayload());
}

void SnakeServer::logOccupiedCells() {
    std::string msg {};
    msg += "Occupied cells (bodies):";
    for (auto & [cell, players] : occupiedCellsBodies) {
        msg += "(" + std::to_string(cell.first) + ", " + std::to_string(cell.second) + "): {";
        for (auto & player : players) {
            msg += std::to_string(player) + " ";
        }
        msg += "}";
    }
    spdlog::debug(msg);

    msg = "";
    msg += "Occupied cells (heads):";
    for (auto & [cell, players] : occupiedCellsHeads) {
        msg += "(" + std::to_string(cell.first) + ", " + std::to_string(cell.second) + "): {";
        for (auto & player : players) {
            msg += std::to_string(player) + " ";
        }
        msg += "}";
    }
    spdlog::debug(msg);
}