#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include "common/Constants.h"

class PlayerNode {
public:
    PlayerNode(int x, int y);
    void move(int xMove, int yMove);
    void moveTo(int xMove, int yMove);
    void grow();
    void getSegments(std::vector<std::pair<int, int>> &);
    int x() {return xPos;};
    int y() {return yPos;};

private:
    int xPos;
    int yPos;
    int prevX;
    int prevY;
    std::unique_ptr<PlayerNode> next { nullptr };
};

inline PlayerNode::PlayerNode(int x, int y)
    : xPos {x}
    , yPos {y} {
}

inline void PlayerNode::move(int xMove, int yMove) {
    moveTo(xPos + xMove, yPos + yMove);
}

inline void PlayerNode::moveTo(int xMove, int yMove) {
    xPos = xMove;
    yPos = yMove;
    if (next != nullptr) {
        next->moveTo(prevX, prevY);
    };
    prevX = xPos;
    prevY = yPos;
}

inline void PlayerNode::grow() {
    if (next == nullptr) {
        next = std::make_unique<PlayerNode>(prevX, prevY);
    }
    else {
        next->grow();
    }
}

inline void PlayerNode::getSegments(std::vector<std::pair<int, int>> & segments) {
    segments.emplace_back(xPos, yPos);
    if (next != nullptr) {
        next->getSegments(segments);
    }
}

struct Player {
    PlayerNode head;
    char direction;
    char nextDirection;
    std::string name;
    int score;
    Color color;
    std::chrono::milliseconds movementFrequencyMs;
    std::chrono::time_point<std::chrono::steady_clock> nextMoveTime;
    bool boosted;
    std::chrono::time_point<std::chrono::steady_clock> boostExpireTime;
};

struct Food {
    int x;
    int y;
    char icon;
    Color color;
};

struct SpeedBoost {
    int x;
    int y;
    char icon;
    Color color;
};
