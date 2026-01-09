#pragma once

#include <string>

inline int MAX_EVENTS {10};
inline int EPOLL_BLOCKING_TIMEOUT {20};
inline int GAME_TICKS_MS {200};

namespace SnakeConstants {
    inline const std::string KEY_QUIT = "q";
    inline const std::string KEY_UP = "^";
    inline const std::string KEY_DOWN = "v";
    inline const std::string KEY_LEFT = "<";
    inline const std::string KEY_RIGHT = ">";
}
