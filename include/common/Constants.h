#pragma once

#include <string>

inline constexpr int SERVER_PORT {8170};
inline constexpr int MAX_EVENTS {10};
inline constexpr int EPOLL_BLOCKING_TIMEOUT {20};
inline constexpr int GAME_TICKS_MS {200};
inline constexpr int ARENA_WIDTH {30};
inline constexpr int ARENA_HEIGHT {30};
inline constexpr int CLIENT_HORIZONTAL_SCALING {2};
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

enum class Color {
    WHITE = 1,
    RED = 2,
    YELLOW = 3,
    GREEN = 4,
    BLUE = 5,
    CYAN = 6,
    MAGENTA = 7,
};
