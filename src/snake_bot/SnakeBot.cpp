#include "snake_bot/SnakeBot.h"

SnakeBot::SnakeBot(const int width, const int height) 
    : width {width}
    , height {height}
    , awaitingJoin {false}
    , clientId {-1}
    , gen {std::random_device{}()}
    , network(getServerIp(), getServerPort())
    , gameState {}
    , pathfinder {width, height} {    
}

void SnakeBot::run() {
    while (true) {
        timer.tick();
        if (clientId == -1 && !awaitingJoin) {
            createBot();
        }
        receiveUpdates();
        if (clientId != -1) {
            sendInput();
        }
    }
}

void SnakeBot::createBot() {
    joinGame();
    awaitingJoin = true;
}

void SnakeBot::joinGame() {
    const char * username = "bot";
    if (!username) username = "unknown";
    spdlog::info("Sending join game request as " + std::string(username, 3));

    network.joinBot(protocol::toString(ProtocolMessage{
        MessageType::CLIENT_JOIN,
        -1,
        username
    }));
    spdlog::info("Sent join game request for " + std::string(username, 3));
}

void SnakeBot::receiveUpdates() {
    std::vector<ProtocolMessage> messages {network.pollMessages()};
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
    spdlog::info("Received server welcome for clientId=" + std::to_string(msg.clientId));
    clientId = msg.clientId;
    awaitingJoin = false;
}

void SnakeBot::handleGameStateMessage(const ProtocolMessage & msg) {
    gameState = client::parseGameState(msg.message);
    if (clientId != -1 && !gameState.players.contains(clientId)) {
        // this means that we just died
        network.destroyBot(clientId);
        clientId = -1;
    }
    buildArenaMap();
}

void SnakeBot::buildArenaMap() {
    pathfinder.rebuildMap(gameState);
}

void SnakeBot::sendInput() {
    if (gameState.players.contains(clientId)) {
        // char input {calculateRandomMove(clientId)};
        const char input {calculatePathingMove(clientId)};
        network.sendToServer(
            clientId,
            protocol::toString(ProtocolMessage {
                MessageType::CLIENT_INPUT,
                clientId,
                std::string(1, input)
            })
        );
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