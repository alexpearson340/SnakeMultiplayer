#include "snake_client/SnakeClient.h"
#include "common/Constants.h"
#include "common/Protocol.h"
#include <cstdlib>
#include <locale.h>
#include <ncurses.h>

SnakeClient::SnakeClient(int width_, int height_)
    : width {width_},
      height {height_},
      running {true},
      playing {false},
      timer {},
      network(getServerIp(), getServerPort()),
      clientId(-1),
      playerInput('\0'),
      gameState {} {}

SnakeClient::~SnakeClient() {
    cleanupNcurses();
}

void SnakeClient::run() {
    initNcurses();
    joinGame();
    while (running) {
        timer.tick();
        handleInput();

        if (playing && playerInput != '\0') {
            sendPlayerInput();
        }
        receiveUpdates();
        render();
    }
}

void SnakeClient::joinGame() {
    const char * username = getenv("USER");
    if (!username) {
        username = "unknown";
    }
    protocol::ClientJoin join {{protocol::MessageType::CLIENT_JOIN, clientId}, {}};
    std::strncpy(join.username, username, sizeof(join.username) - 1);
    network.sendToServer({protocol::serialise(join)});
}

void SnakeClient::handleInput() {
    int ch = getch();
    if (ch != ERR) {
        if (ch == 'q' || ch == 'Q') {
            running = false;
        } else if (!playing && (ch == 'r' || ch == 'R')) {
            joinGame();
        } else if (ch == KEY_UP) {
            playerInput = '^';
        } else if (ch == KEY_DOWN) {
            playerInput = 'v';
        } else if (ch == KEY_LEFT) {
            playerInput = '<';
        } else if (ch == KEY_RIGHT) {
            playerInput = '>';
        }
    }
    flushinp(); // Flush any remaining input
}

void SnakeClient::sendPlayerInput() {
    network.sendToServer(
        {protocol::serialise(protocol::ClientInput {{protocol::MessageType::CLIENT_INPUT, clientId}, playerInput})});
    playerInput = '\0';
}

void SnakeClient::receiveUpdates() {
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

void SnakeClient::handleServerWelcome(const protocol::ServerWelcome & msg) {
    clientId = msg.hdr.clientId;
    playing = true;
}

void SnakeClient::handleGameStateMessage(const protocol::GameState & msg) {
    gameState = client::fromProtocol(msg);
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
    for (auto & [id, p] : gameState.players) {
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
    std::string serverHighScore {"Server high score is " + gameState.serverHighScore.first + ": " +
                                 std::to_string(gameState.serverHighScore.second)};
    mvprintw(height + 4, 0, "%s", serverHighScore.c_str());
    std::vector<client::PlayerData> sortedPlayers {};
    for (auto & [id, p] : gameState.players) {
        sortedPlayers.push_back(p);
    }

    // Sort by score descending
    std::sort(sortedPlayers.begin(), sortedPlayers.end(),
              [](const auto & l, const auto & r) { return l.score > r.score; });

    int row = height + 6;
    mvprintw(row++, 0, "+----------- SCOREBOARD -----------+");

    int playersPrintedToScoreboard {0};
    for (auto & p : sortedPlayers) {
        mvprintw(row, 0, "| ");
        mvaddch(row, 2, '#' | COLOR_PAIR(p.color));
        mvprintw(row, 4, "%-22s %7d |", p.name.c_str(), p.score);
        row++;
        playersPrintedToScoreboard++;
        if (playersPrintedToScoreboard == 5) {
            break;
        }
    }

    mvprintw(row, 0, "+----------------------------------+");
}

void SnakeClient::renderCharToScreen(const int x, const int y, const char & character, const int color) {
    mvaddch(y, x * CLIENT_HORIZONTAL_SCALING,
            static_cast<chtype>(static_cast<unsigned char>(character)) | COLOR_PAIR(color));
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
