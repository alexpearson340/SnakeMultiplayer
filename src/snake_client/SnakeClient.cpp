#include <ncurses.h>
#include "engine/ProtocolMessage.h"
#include "snake_client/SnakeClient.h"

SnakeClient::SnakeClient(int width, int height)
    : Engine(width, height)
    , network("127.0.0.1", 8170)
    , clientId(-1)
    , playerInput('^') {
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

void SnakeClient::create() {
    initNcurses();

    // send a CLIENT_JOIN message to the server
    ProtocolMessage clientJoinMessage {
        MessageType::CLIENT_JOIN,
        clientId,
        "apearson"  // todo
    };
    network.sendToServer(protocol::toString(clientJoinMessage));
}

void SnakeClient::update() {
    // send the latest player input to the server if successfully joined game
    if (clientId != -1) {
        sendPlayerInput();
    }

    // receive welcome message + latest game state from server
    receiveUpdates();
}

void SnakeClient::render() {
    clear();

    for (int y = 0; y <= height + 1; y++) {
        for (int x = 0; x <= width + 1; x++) {
            if (x == 0 || y == 0 || x == width + 1 || y == height + 1) {
                mvaddch(y, x, '.');  // boundary
            }
            else {
                mvaddch(y, x, ' ');  // empty space
            }
        }
    }

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
    for (auto msg : messages) {
        switch (msg.messageType) {
            case MessageType::SERVER_WELCOME:
                handleServerWelcome(msg);
                break;
            case MessageType::GAME_STATE:
                handleGameStateMessage(msg);
                break;
            default:
                throw std::runtime_error("Invalid MessageType");
        }
    }
}

void SnakeClient::handleServerWelcome(const ProtocolMessage & msg) {
    clientId = msg.clientId;
}

void SnakeClient::handleGameStateMessage(const ProtocolMessage & msg) {
}
