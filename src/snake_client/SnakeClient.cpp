#include <locale.h>
#include <ncurses.h>
#include <cstdlib>
#include "common/Constants.h"
#include "common/ProtocolMessage.h"
#include "snake_client/SnakeClient.h"

SnakeClient::SnakeClient(int width, int height)
    : width {width}
    , height {height}
    , running {true}
    , playing {false}
    , score {0}
    , network(getServerIp(), getServerPort())
    , clientId(-1)
    , playerInput('\0')
    , gameState{} {
}

SnakeClient::~SnakeClient() {
    cleanupNcurses();
}

void SnakeClient::run() {
    initNcurses();
    joinGame();
    while (running) {
        handleInput();

        if (playing && playerInput != '\0') {
            sendPlayerInput();
        }
        receiveUpdates();
        render();
    }
}

void SnakeClient::joinGame() {
    const char* username = getenv("USER");
    if (!username) username = "unknown";

    ProtocolMessage clientJoinMessage {
        MessageType::CLIENT_JOIN,
        clientId,
        username
    };
    network.sendToServer(protocol::toString(clientJoinMessage));
}

void SnakeClient::handleInput() {
    int ch = getch();
    if (ch != ERR) {
        if (ch == 'q' || ch == 'Q') {
            running = false;
        }
        else if (!playing && (ch == 'r' || ch == 'R')) {
            joinGame();
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

void SnakeClient::sendPlayerInput() {
    ProtocolMessage playerInputMessage {
        MessageType::CLIENT_INPUT,
        clientId,
        std::string(1, playerInput)
    };
    network.sendToServer(protocol::toString(playerInputMessage));
    playerInput = '\0';
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
    playing = true;
}

void SnakeClient::handleGameStateMessage(const ProtocolMessage & msg) {
    gameState = client::parseGameState(msg.message);
    if (!gameState.players.contains(clientId)) {
        playing = false;
        clientId = -1;
    }
}

void SnakeClient::render() {
    erase();
    renderArena();
    renderObjects();
    renderPlayers();
    renderScore();
    refresh();
}

void SnakeClient::renderArena() {
    // top and bottom boundary
    for (int x = 1; x <= width; x++) {
        renderCharToScreen(x, 0, '-');
        renderCharToScreen(x, height + 1, '-');
    }

    // left and right boundary
    for (int y = 1; y <= height; y++) {
        renderCharToScreen(0, y, '|');
        renderCharToScreen(width + 1, y, '|');
    }

    // corners
    renderCharToScreen(0, 0, '+');
    renderCharToScreen(width + 1, 0, '+');
    renderCharToScreen(0, height + 1, '+');
    renderCharToScreen(width + 1, height + 1, '+');
}

void SnakeClient::renderPlayers() {
    for (auto & [clientId, p] : gameState.players) {
        renderCharToScreen(p.segments[0].first, p.segments[0].second, p.direction);
        for (auto it = p.segments.begin() + 1; it < p.segments.end(); it++) {
            renderCharToScreen(it->first, it->second, 'c', p.color);
        }
    }
}

void SnakeClient::renderObjects() {
    for (auto & f : gameState.food) {
        renderCharToScreen(f.x, f.y, f.icon, f.color);
    }
    for (auto & s : gameState.speedBoosts) {
        renderCharToScreen(s.x, s.y, s.icon, s.color);
    }
}

void SnakeClient::renderScore() {
    mvprintw(height + 2, 0, "Press 'q' to quit");
    mvprintw(height + 3, 0, "Press 'r' to reload");
    std::string serverHighScore {"Server high score is " + gameState.serverHighScore.first + ": " + std::to_string(gameState.serverHighScore.second)};
    mvprintw(height + 4, 0, serverHighScore.c_str());
    std::vector<client::PlayerData> sortedPlayers {};
    for (auto & [clientId, p] : gameState.players) {
        sortedPlayers.push_back(p);
    }

    // Sort by score descending
    std::sort(sortedPlayers.begin(), sortedPlayers.end(),
        [](const auto & l, const auto & r) {return l.score > r.score;});

    int row = height + 6;
    mvprintw(row++, 0, "+----------- SCOREBOARD -----------+");

    for (auto & p : sortedPlayers) {
        mvprintw(row, 0, "| ");
        mvaddch(row, 2, '#' | COLOR_PAIR(p.color));
        mvprintw(row, 4, "%-22s %7d |", p.name.c_str(), p.score);
        row++;
    }

    mvprintw(row, 0, "+----------------------------------+");
}

void SnakeClient::renderCharToScreen(const int x, const int y, const char & character, const int color) {
    mvaddch(y, x * CLIENT_HORIZONTAL_SCALING, character | COLOR_PAIR(color));
}

void SnakeClient::initNcurses() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    timeout(INPUT_BLOCKING_TIMEOUT_MS);
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    init_pair(static_cast<int>(Color::WHITE), COLOR_WHITE, COLOR_BLACK);
    init_pair(static_cast<int>(Color::YELLOW), COLOR_YELLOW, COLOR_BLACK);
    init_pair(static_cast<int>(Color::RED), COLOR_RED, COLOR_BLACK);
    init_pair(static_cast<int>(Color::GREEN), COLOR_GREEN, COLOR_BLACK);
    init_pair(static_cast<int>(Color::CYAN), COLOR_CYAN, COLOR_BLACK);
    init_pair(static_cast<int>(Color::MAGENTA), COLOR_MAGENTA, COLOR_BLACK);
}

void SnakeClient::cleanupNcurses() {
    endwin();
}
