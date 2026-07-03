#pragma once

#include "snake_client/GameState.h"
#include <deque>
#include <vector>

class Pathfinder {
public:
    Pathfinder(const int width_, const int height_);
    char calculateNextMove(const int, const client::GameState &) const;
    void rebuildMap(const client::GameState &);

private:
    void populateFoodAndPlayers(const client::GameState &);
    void computePaths(const client::GameState &);
    void checkNeighbour(const int, const int, const int, std::deque<std::pair<int, int>> &);
    int & cell(int x, int y);
    const int & cell(int x, int y) const;
    const int width;
    const int height;
    std::vector<std::vector<int>> dijkstraMap;
};