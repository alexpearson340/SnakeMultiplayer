#include "snake_bot/SnakeBot.h"
#include <cstddef>

SnakeBot::SnakeBot(const int width, const int height)
    : awaitingJoin {false},
      gameStateHasChanged {true},
      clientId {-1},
      gen {std::random_device {}()},
      network(getServerIp(), getServerPort()),
      gameState {},
      pathfinder {width, height} {}

void SnakeBot::run() {
    while (true) {
        network.waitForReadable(EPOLL_BLOCKING_TIMEOUT_MS);
        timer.tick();
        if (clientId == -1 && !awaitingJoin) {
            createBot();
        }
        receiveUpdates();
        if (clientId != -1) {
            if (gameStateHasChanged) {
                sendInput();
            };
            gameStateHasChanged = false;
        }
    }
}

void SnakeBot::createBot() {
    joinGame();
    awaitingJoin = true;
}

void SnakeBot::joinGame() {
    const char * username = "bot";
    if (!username)
        username = "unknown";
    spdlog::info("Sending join game request as " + std::string(username, 3));

    network.sendToServer({jsonprotocol::toString({MessageType::CLIENT_JOIN, username, clientId})});
    spdlog::info("Sent join game request for " + std::string(username, 3));
}

void SnakeBot::receiveUpdates() {
    std::vector<Bytes> messages {network.receiveFromServer()};
    std::optional<ProtocolMessage> latestGameState;

    for (auto & msgBytes : messages) {
        ProtocolMessage msg {jsonprotocol::fromString(msgBytes)};
        switch (msg.messageType) {
        case MessageType::SERVER_WELCOME:
            handleServerWelcome(msg);
            break;
        case MessageType::GAME_STATE:
            latestGameState = std::move(msg);
            gameStateHasChanged = true;
            break;
        default:
            throw std::runtime_error("Invalid MessageType");
        }
    }

    // We only process the latest GAME_STATE message, to avoid
    // getting behind on the client side when all we care about
    // is the latest GAME_STATE anyway
    if (latestGameState) {
        handleGameStateMessage(latestGameState.value());
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
        clientId = -1;
    }
    buildArenaMap();
}

void SnakeBot::buildArenaMap() {
    pathfinder.rebuildMap(gameState);
}

void SnakeBot::sendInput() {
    if (gameState.players.contains(clientId)) {
        // char input {calculateRandomMove()};
        const char input {calculatePathingMove()};
        network.sendToServer({jsonprotocol::toString({MessageType::CLIENT_INPUT, std::string(1, input), clientId})});
    }
}

char SnakeBot::calculateRandomMove() {
    std::vector<char> possibleDirections {'<', '^', '>', 'v'};
    std::uniform_int_distribution<std::size_t> dist(0, possibleDirections.size() - 1);
    return possibleDirections[dist(gen)];
}

char SnakeBot::calculatePathingMove() const {
    return pathfinder.calculateNextMove(clientId, gameState);
}