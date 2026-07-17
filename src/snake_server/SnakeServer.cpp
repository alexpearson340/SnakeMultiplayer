#include "snake_server/SnakeServer.h"
#include "common/Constants.h"
#include "common/Json.h"
#include "common/Log.h"
#include "common/MessageLogWriter.h"
#include <chrono>
#include <stdexcept>
#include <string>

SnakeServer::SnakeServer(const ServerConfig & config, std::optional<MessageLogReader> && reader)
    : width {config.width},
      height {config.height},
      currentSequence {0},
      movementFrequencyMs(config.movementFrequencyMs),
      boostedMovementFrequencyMs(config.boostedMovementFrequencyMs),
      boostDurationMs(config.boostDurationMs),
      timer {},
      seed {config.seed},
      gen {seed},
      msgLogWriter {config.applicationName},
      serverHighScore {},
      replayFile {std::move(reader)},
      network {config.port},
      clientIdToPlayerMap {},
      occupiedCellsBodies {},
      occupiedCellsHeads {},
      foodMap {},
      speedBoostMap {} {}

void SnakeServer::run() {
    recordServerConfig();
    const std::chrono::time_point<std::chrono::steady_clock> start {std::chrono::steady_clock::now()};
    int64_t ticks {0};

    while (std::optional<std::vector<ProtocolMessage>> messages = pollMessages()) {
        ticks++;
        replaceFood();
        bool stateChanged = false;
        for (auto & msg : messages.value()) {
            stampMessage(msg);
            std::string msgBytes {protocol::toString(msg)};
            msgLogWriter.log(msgBytes);
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
    logEngineBenchmark(start, ticks);
}

void SnakeServer::recordServerConfig() {
    json sessionConfig;
    sessionConfig["width"] = width;
    sessionConfig["height"] = height;
    sessionConfig["seed"] = seed;
    sessionConfig["movement_frequency_ms"] = movementFrequencyMs.count();
    sessionConfig["boosted_movement_frequency_ms"] = boostedMovementFrequencyMs.count();
    sessionConfig["boost_duration_ms"] = boostDurationMs.count();
    sessionConfig["min_food_in_arena"] = MIN_FOOD_IN_ARENA;
    sessionConfig["food_spawn_from_body_segment_probability"] = FOOD_SPAWN_FROM_BODY_SEGMENT_PROBABILITY;
    sessionConfig["speed_boost_probability"] = SPEED_BOOST_PROBABILITY;
    sessionConfig["speed_boost_ratio"] = SPEED_BOOST_RATIO;
    ProtocolMessage pm {MessageType::SERVER_CONFIG, sessionConfig.dump()};
    stampMessage(pm);
    std::string msgBytes {protocol::toString(pm)};
    msgLogWriter.log(msgBytes);
}

void SnakeServer::stampMessage(ProtocolMessage & msg) {
    msg.sequence = currentSequence++;
    msg.transactTime = timer.currentTickAsNanos();
}

bool SnakeServer::isInReplay() const {
    return replayFile.has_value();
}

std::optional<std::vector<ProtocolMessage>> SnakeServer::pollMessages() {
    std::vector<ProtocolMessage> messages;
    if (isInReplay()) {
        std::vector<ProtocolMessage> fileMessages {replayFile->nextBatch()};
        if (!fileMessages.empty()) {
            for (auto & pm : fileMessages) {
                timer.setTick(pm.transactTime);
                if (pm.messageType != MessageType::SERVER_WELCOME && pm.messageType != MessageType::GAME_STATE) {
                    messages.push_back(pm);
                }
            }
        } else {
            spdlog::info("Exiting replay mode currentSequence=" + std::to_string(currentSequence));
            replayFile.reset();
            return std::nullopt;
        }
    } else {
        std::vector<std::pair<int, Bytes>> networkMessages {network.pollMessages()};
        for (auto & [clientId, frame] : networkMessages) {
            messages.push_back(protocol::fromString(frame, clientId));
        }
        timer.tick();
    }

    // check for client disconnects and sythesise the messages we need
    for (int clientId : network.drainDisconnects()) {
        messages.push_back({MessageType::CLIENT_DISCONNECT, "", clientId});
    }
    return messages;
}

void SnakeServer::handleClientJoin(const ProtocolMessage & msg) {
    spdlog::info("Received client join request from " + msg.message);
    createNewPlayer(msg);

    // send a SERVER_WELCOME message back to the client, confirming that they are playing
    ProtocolMessage pm {MessageType::SERVER_WELCOME, "welcome " + msg.message, msg.clientId};
    stampMessage(pm);
    std::string msgBytes {protocol::toString(pm)};
    msgLogWriter.log(msgBytes);
    if (!isInReplay()) {
        network.sendToClient(msg.clientId, msgBytes);
    }
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
    } else if (msg.message == SnakeConstants::PLAYER_KEY_DOWN) {
        if (player.direction != '^') {
            player.nextDirection = 'v';
        }
    } else if (msg.message == SnakeConstants::PLAYER_KEY_LEFT) {
        if (player.direction != '>') {
            player.nextDirection = '<';
        }
    } else if (msg.message == SnakeConstants::PLAYER_KEY_RIGHT) {
        if (player.direction != '<') {
            player.nextDirection = '>';
        }
    } else {
        spdlog::info("Unexpected receive from clientId(" + std::to_string(msg.clientId) + "): " + msg.message);
    }
}

void SnakeServer::createNewPlayer(const ProtocolMessage & msg) {
    std::uniform_int_distribution<> distX(1 + 5, width - 1 - 5);
    std::uniform_int_distribution<> distY(1 + 5, height - 1 - 5);
    clientIdToPlayerMap.emplace(msg.clientId,
                                Player {PlayerNode {distX(gen), distY(gen)}, '^', '^', msg.message, 1,
                                        static_cast<Color>((msg.clientId % 5) + 2), movementFrequencyMs,
                                        timer.currentTick() + movementFrequencyMs, false, timer.currentTick()});
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
        const auto & playerHeadCells {occupiedCellsHeads.at(playerHead)};

        // collision with arena boundary
        if (player.head.y() <= 0) {
            spdlog::info("Destroying " + player.name + " due to upper boundary collision");
            clientIdsToDestroy.push_back(clientId);
        } else if (player.head.y() >= height + 1) {
            spdlog::info("Destroying " + player.name + " due to lower boundary collision");
            clientIdsToDestroy.push_back(clientId);
        } else if (player.head.x() <= 0) {
            spdlog::info("Destroying " + player.name + " due to left boundary collision");
            clientIdsToDestroy.push_back(clientId);
        } else if (player.head.x() >= width + 1) {
            spdlog::info("Destroying " + player.name + " due to upper boundary collision");
            clientIdsToDestroy.push_back(clientId);
        }

        // collision with another snake's body
        else if (occupiedCellsBodies.contains(playerHead)) {
            spdlog::info("Destroying " + player.name + " due to snake body collision");
            clientIdsToDestroy.push_back(clientId);
        }

        // head-on-head snake collision - whoever arrived into the cell first survives
        else if (playerHeadCells.size() > 1) {
            int firstPlayerinCell {*std::min_element(
                playerHeadCells.begin(), playerHeadCells.end(),
                // Checking who was first in the cell based on nextMoveTime is slightly imperfect.
                // A client with a faster move speed can arrive later and still have a lower nextMoveTime.
                // This is probably good enough though
                [this](const int a, const int b) {
                    return clientIdToPlayerMap.at(a).nextMoveTime < clientIdToPlayerMap.at(b).nextMoveTime;
                })};
            if (clientId != firstPlayerinCell) {
                spdlog::info("Destroying " + player.name + " due to snake head collision");
                clientIdsToDestroy.push_back(clientId);
            }
        }

        // get food
        else if (foodMap.contains(playerHead)) {
            spdlog::debug("Feeding player " + player.name + " at " + "(" + std::to_string(playerHead.first) + ", " +
                          std::to_string(playerHead.second) + ")");
            feedPlayer(playerHead, clientId);
        }

        // get speed boost
        else if (speedBoostMap.contains(playerHead)) {
            spdlog::debug("Boosting player " + player.name + " at " + "(" + std::to_string(playerHead.first) + ", " +
                          std::to_string(playerHead.second) + ")");
            boostPlayer(playerHead, clientId);
        }

        // track all time score
        if (player.score > serverHighScore.second) {
            serverHighScore = {player.name, player.score};
            spdlog::debug("New server high score, " + player.name + ": " + std::to_string(player.score));
        }
    }
    if (!clientIdsToDestroy.empty()) {
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
    while (foodMap.size() < MIN_FOOD_IN_ARENA) {
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
    ProtocolMessage pm {MessageType::GAME_STATE, buildGameStatePayload()};
    stampMessage(pm);
    std::string msgBytes {protocol::toString(pm)};
    msgLogWriter.log(msgBytes);
    if (!isInReplay()) {
        network.broadcast(msgBytes);
    }
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

void SnakeServer::logEngineBenchmark(const std::chrono::time_point<std::chrono::steady_clock> & start,
                                     const int64_t & ticks) {
    const int64_t ns {
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count()};
    const double engineMs {static_cast<double>(ns) / 1.0e6};
    const double usPerTick {ticks ? static_cast<double>(ns) / 1000.0 / static_cast<double>(ticks) : 0.0};
    spdlog::info("BENCH ticks={} engine_ms={:.3f} us_per_tick={:.3f}", ticks, engineMs, usPerTick);
}
