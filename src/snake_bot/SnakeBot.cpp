#include <chrono>
#include <thread>
#include "snake_bot/SnakeBot.h"

SnakeBot::SnakeBot(const int width, const int height) 
    : width {width}
    , height {height}
    , awaitingJoin {false}
    , isAlive {false}
    , clientId {-1}
    , gen {std::random_device{}()}
    , network(getServerIp(), getServerPort())
    , gameState {}
    , pathfinder {width, height} {    
}

void SnakeBot::run() {
    while (true) {
        if (!isAlive && !awaitingJoin) {
            createBot();
        }
        receiveUpdates();
        buildArenaMap();
        sendInput();

        // todo remove this when we wait responsively on server socket input
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SnakeBot::createBot() {
    joinGame();
    awaitingJoin = true;
}

void SnakeBot::joinGame() {
    const char * username = getenv("USER");
    if (!username) username = "unknown";

    ProtocolMessage clientJoinMessage {
        MessageType::CLIENT_JOIN,
        clientId,
        username
    };
    network.sendToServer(protocol::toString(clientJoinMessage));
}

void SnakeBot::receiveUpdates() {
    std::vector<ProtocolMessage> messages {network.receiveFromServer()};
    ProtocolMessage * latestGameState {nullptr};

    for (auto & msg : messages) {
        switch (msg.messageType) {
            case MessageType::SERVER_WELCOME:
                handleServerWelcome(msg);
                break;
            case MessageType::GAME_STATE:
                latestGameState = &msg;
                break;
            default:
                throw std::runtime_error("Invalid MessageType");
        }
    }

    // We only process the latest GAME_STATE message, to avoid
    // getting behind on the client side when all we care about
    // is the latest GAME_STATE anyway
    if (latestGameState != nullptr) {
        handleGameStateMessage(*latestGameState);
    }
}

void SnakeBot::handleServerWelcome(const ProtocolMessage & msg) {
    clientId = msg.clientId;
    awaitingJoin = false;
    isAlive = true;
}

void SnakeBot::handleGameStateMessage(const ProtocolMessage & msg) {
    gameState = client::parseGameState(msg.message);
    if (!gameState.players.contains(clientId)) {
        clientId = -1;
        isAlive = false;
    }
}

void SnakeBot::buildArenaMap() {
    pathfinder.rebuildMap(gameState);
}

void SnakeBot::sendInput() {
    if (clientId != -1) {
        // char input {calculateRandomMove(clientId)};
        const char input {calculatePathingMove(clientId)};
        network.sendToServer(protocol::toString(ProtocolMessage {
            MessageType::CLIENT_INPUT,
            clientId,
            std::string(1, input)
        }));
    }
}

const char SnakeBot::calculateRandomMove(const int clientId) {
    std::vector<char> possibleDirections {'<', '^', '>', 'v'};
    std::uniform_int_distribution<> dist (1, 4);
    return possibleDirections[dist(gen) - 1];
}

const char SnakeBot::calculatePathingMove(const int clientId) const {
    return pathfinder.calculateNextMove(clientId, gameState);
}