#pragma once

#include <string>

inline const std::string SERVER_IP_ADDRESS {"127.0.0.1"};
inline constexpr int SERVER_PORT {8170};
inline constexpr int MAX_EVENTS {10};
inline constexpr int EPOLL_BLOCKING_TIMEOUT {20};
inline constexpr int GAME_TICKS_MS {150};
inline constexpr int ARENA_WIDTH {80};
inline constexpr int ARENA_HEIGHT {30};
inline constexpr int MIN_FOOD_IN_ARENA {3};

namespace SnakeConstants {
    inline const std::string KEY_QUIT = "q";
    inline const std::string KEY_UP = "^";
    inline const std::string KEY_DOWN = "v";
    inline const std::string KEY_LEFT = "<";
    inline const std::string KEY_RIGHT = ">";
    inline const std::string PLAYER_KEY_QUIT = "q";
    inline const std::string PLAYER_KEY_UP = "^";
    inline const std::string PLAYER_KEY_DOWN = "v";
    inline const std::string PLAYER_KEY_LEFT = "<";
    inline const std::string PLAYER_KEY_RIGHT = ">";
}
