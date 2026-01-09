#include <ncurses.h>
#include <chrono>
#include <thread>
#include "common/ProtocolMessage.h"
#include "snake_client/SnakeClient.h"

SnakeClient::SnakeClient(int width, int height)
    : width {width}
    , height {height}
    , running {true}
    , score {0}
    , network("127.0.0.1", 8170)
    , clientId(-1)
    , playerInput('\0')
    , gameState{} {
}

SnakeClient::~SnakeClient() {
    cleanupNcurses();
}

void SnakeClient::handleInput() {
    int ch = getch();
    if (ch != ERR) {
        if (ch == 'q' || ch == 'Q') {
            running = false;
        }
        else if (ch == KEY_UP) {
            playerInput = '^';
        }
        else if (ch == KEY_DOWN) {
            playerInput = 'v';
        }
        else if (ch == KEY_LEFT) {
            playerInput = '<';
        }
        else if (ch == KEY_RIGHT) {
            playerInput = '>';
        }
    }
    flushinp();  // Flush any remaining input
}

void SnakeClient::run() {
    initNcurses();

    // send a CLIENT_JOIN message to the server
    ProtocolMessage clientJoinMessage {
        MessageType::CLIENT_JOIN,
        clientId,
        "apearson"  // todo
    };
    network.sendToServer(protocol::toString(clientJoinMessage));

    while (running) {
        handleInput();

        if (clientId != -1) {
            sendPlayerInput();
        }

        receiveUpdates();
        render();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SnakeClient::render() {
    clear();
    renderArena();
    renderPlayers();

    mvprintw(height + 2, 0, "Score:%d, Width: %d, Height:%d", score, width, height);
    mvprintw(height + 3, 0, "Press 'q' to quit.\\n");

    refresh();
}

void SnakeClient::cleanup() {
}

void SnakeClient::initNcurses() {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);
}

void SnakeClient::cleanupNcurses() {
    endwin();
}

void SnakeClient::sendPlayerInput() {
    ProtocolMessage playerInputMessage {
        MessageType::CLIENT_INPUT,
        clientId,
        std::string(1, playerInput)
    };
    network.sendToServer(protocol::toString(playerInputMessage));
}

void SnakeClient::receiveUpdates() {
    std::vector<ProtocolMessage> messages { network.receiveFromServer() };
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

void SnakeClient::handleServerWelcome(const ProtocolMessage & msg) {
    clientId = msg.clientId;
}

void SnakeClient::handleGameStateMessage(const ProtocolMessage & msg) {
    gameState = client::parseGameState(msg.message);
}

void SnakeClient::renderArena() {
    for (int y = 0; y <= height + 1; y++) {
        for (int x = 0; x <= width + 1; x++) {
            if (x == 0 || y == 0 || x == width + 1 || y == height + 1) {
                mvaddch(y, x, '.');  // boundary
            }
        }
    }
}

void SnakeClient::renderPlayers() {
    for (auto & p : gameState.players) {
        mvaddch(p.segments[0].second, p.segments[0].first, p.direction);
        for (auto it = p.segments.begin() + 1; it < p.segments.end(); it++) {
            mvaddch(it->second, it->first, '.');
        }
    }
}

void SnakeClient::renderFood() {
}

void SnakeClient::renderScore() {
}
