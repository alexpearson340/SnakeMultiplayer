#include <memory>

class PlayerNode {
public:
    PlayerNode(int x, int y);
    void move(int xMove, int yMove);
    void moveTo(int xMove, int yMove);
    bool isInSnakeBody(int x, int y);
    void grow();
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

inline bool PlayerNode::isInSnakeBody(int x, int y) {
    PlayerNode * nextBodyPart { next.get() };

    while (nextBodyPart != nullptr) {
        if (nextBodyPart->x() == x && nextBodyPart->y() == y) {
            return true;
        }
        nextBodyPart = nextBodyPart->next.get();
    }
    return false;
}

inline void PlayerNode::grow() {
    if (next == nullptr) {
        next = std::make_unique<PlayerNode>(prevX, prevY);
    }
    else {
        next->grow();
    }
}

struct Player {
    PlayerNode head;
    char direction;
    std::string name;
};
