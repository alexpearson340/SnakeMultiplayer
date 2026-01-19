#include "snake_bot/Pathfinder.h"
#include <climits>
#include <iostream>

Pathfinder::Pathfinder(const int width, const int height)
    : width {width}
    , height {height}
    , dijkstraMap {} {
}

const char Pathfinder::calculateNextMove(const int clientId, const client::GameState & gameState) const {
    const char & direction {gameState.players.at(clientId).direction};
    const int & x {gameState.players.at(clientId).segments[0].first};
    const int & y {gameState.players.at(clientId).segments[0].second};

    std::pair<long, char> lowest {LONG_MAX, '?'};
    if (direction != 'v' && y > 1) {
        const int & neighbourVal {dijkstraMap.at(y - 1).at(x)};
        if (neighbourVal >= 0 && neighbourVal < lowest.first) {
            lowest.first = neighbourVal;
            lowest.second = '^';
        }
    }
    if (direction != '^' && y < height) {
        const int & neighbourVal {dijkstraMap.at(y + 1).at(x)};
        if (neighbourVal >= 0 && neighbourVal < lowest.first) {
            lowest.first = neighbourVal;
            lowest.second = 'v';
        }
    }
    if (direction != '>' && x > 1) {
        const int & neighbourVal {dijkstraMap.at(y).at(x - 1)};
        if (neighbourVal >= 0 && neighbourVal < lowest.first) {
            lowest.first = neighbourVal;
            lowest.second = '<';
        }
    }
    if (direction != '<' && x < width) {
        const int & neighbourVal {dijkstraMap.at(y).at(x + 1)};
        if (neighbourVal >= 0 && neighbourVal < lowest.first) {
            lowest.first = neighbourVal;
            lowest.second = '>';
        }
    }
    return lowest.second;
}

void Pathfinder::rebuildMap(const client::GameState & gameState) {
    // clear the map out
    dijkstraMap.assign(height + 1, std::vector<int>(width + 1, INT_MAX));
    populateFoodAndPlayers(gameState);
    computePaths(gameState);
}

void Pathfinder::populateFoodAndPlayers(const client::GameState & gameState) {
    // populate food and boosts as 0 - the goal for pathfinding
    for (auto & f : gameState.food) {
        dijkstraMap.at(f.y).at(f.x) = 0;
    }
    for (auto & b : gameState.speedBoosts) {
        dijkstraMap.at(b.y).at(b.x) = 0;
    }

    // populate player head and body segments as negative - considered impassable
    for (auto & [clientId, player] : gameState.players) {
        for (auto & s : player.segments) {
            dijkstraMap.at(s.second).at(s.first) = -1;
        }
    }
}

void Pathfinder::computePaths(const client::GameState & gameState) {
    std::deque<std::pair<int, int>> toVisit {};
    for (auto & f : gameState.food) {
        if (dijkstraMap.at(f.y).at(f.x) == 0) {     // dont seed on food hidden underneath a body part
            toVisit.push_back({f.x, f.y});
        }
    }
    for (auto & b : gameState.speedBoosts) {
        if (dijkstraMap.at(b.y).at(b.x) == 0) {
            toVisit.push_back({b.x, b.y});
        }
    }

    std::pair<int, int> currentCell;
    int nextVal;

    // we keep searching until there is nowhere else to visit
    while (!toVisit.empty()) {
        currentCell = std::move(toVisit.front());
        toVisit.pop_front();
        nextVal = dijkstraMap.at(currentCell.second).at(currentCell.first) + 1;

        checkNeighbour(nextVal, currentCell.first, currentCell.second - 1, toVisit);    // upwards cell
        checkNeighbour(nextVal, currentCell.first, currentCell.second + 1, toVisit);    // downwards cell
        checkNeighbour(nextVal, currentCell.first - 1, currentCell.second, toVisit);    // left cell
        checkNeighbour(nextVal, currentCell.first + 1, currentCell.second, toVisit);    // right cell
    }
}

void Pathfinder::checkNeighbour(const int nextVal, const int neighbourX, const int neighbourY, std::deque<std::pair<int, int>> & toVisit) {
    // neighbour is out of bounds
    if ((neighbourX <= 0) || (neighbourX > width) || (neighbourY <= 0) || (neighbourY > height)) {
        return;
    }

    // neighbour has already been visited or is impassable
    if (dijkstraMap.at(neighbourY).at(neighbourX) != INT_MAX) {
        return;
    }

    dijkstraMap.at(neighbourY).at(neighbourX) = nextVal;
    toVisit.push_back({neighbourX, neighbourY});
}