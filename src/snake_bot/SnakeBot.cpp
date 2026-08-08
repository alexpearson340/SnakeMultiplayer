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
    if (!username) {
        username = "unknown";
    }
    spdlog::info("Sending join game request as " + std::string(username, 3));

    protocol::ClientJoin join {{protocol::MessageType::CLIENT_JOIN, clientId}, {}};
    std::strncpy(join.username, username, sizeof(join.username) - 1);
    network.sendToServer({protocol::serialise(join)});
    spdlog::info("Sent join game request for " + std::string(username, 3));
}

void SnakeBot::receiveUpdates() {
    std::vector<Bytes> messages {network.receiveFromServer()};
    std::optional<protocol::GameState> latestGameState;

    for (auto & msgBytes : messages) {
        protocol::MessageVariant msg {protocol::deserialise(msgBytes)};
        switch (protocol::header(msg).messageType) {
        case protocol::MessageType::SERVER_WELCOME:
            handleServerWelcome(std::get<protocol::ServerWelcome>(msg));
            break;
        case protocol::MessageType::GAME_STATE:
            latestGameState = std::move(std::get<protocol::GameState>(msg));
            gameStateHasChanged = true;
            break;
        default:
            throw std::runtime_error("Invalid protocol::MessageType");
        }
    }

    // We only process the latest GAME_STATE message, to avoid
    // getting behind on the client side when all we care about
    // is the latest GAME_STATE anyway
    if (latestGameState) {
        handleGameStateMessage(latestGameState.value());
    }
}

void SnakeBot::handleServerWelcome(const protocol::ServerWelcome & msg) {
    spdlog::info("Received server welcome for clientId=" + std::to_string(msg.hdr.clientId));
    clientId = msg.hdr.clientId;
    awaitingJoin = false;
}

void SnakeBot::handleGameStateMessage(const protocol::GameState & msg) {
    gameState = client::fromProtocol(msg);
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
        network.sendToServer(
            {protocol::serialise(protocol::ClientInput {{protocol::MessageType::CLIENT_INPUT, clientId}, input})});
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