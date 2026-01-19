#pragma once

#include <deque>
#include <vector>
#include "snake_client/GameState.h"

class Pathfinder {
public:
    Pathfinder(const int width, const int height);
    const char calculateNextMove(const int, const client::GameState &) const;
    void rebuildMap(const client::GameState &);

private:
    void populateFoodAndPlayers(const client::GameState &);
    void computePaths(const client::GameState &);
    void checkNeighbour(const int, const int, const int, std::deque<std::pair<int, int>> &);
    const int width;
    const int height;
    std::vector<std::vector<int>> dijkstraMap;
};