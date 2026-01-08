#include <ncurses.h>
#include "snake_client/SnakeClient.h"

SnakeClient::SnakeClient(int width, int height)
    : Engine(width, height) {
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
        }
        else if (ch == KEY_DOWN) {
        }
        else if (ch == KEY_LEFT) {
        }
        else if (ch == KEY_RIGHT) {
        }
    }
    flushinp();  // Flush any remaining input
}

void SnakeClient::create() {
    initNcurses();
}

void SnakeClient::update() {
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
