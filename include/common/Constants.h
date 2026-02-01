#pragma once

#include <string>

inline constexpr int SERVER_PORT {8170};
inline constexpr int MAX_EVENTS {10};
inline constexpr int EPOLL_BLOCKING_TIMEOUT_MS {10};
inline constexpr int INPUT_BLOCKING_TIMEOUT_MS {10};
inline constexpr int MOVEMENT_FREQUENCY_MS {200};
inline constexpr int ARENA_WIDTH {40};
inline constexpr int ARENA_HEIGHT {40};
inline constexpr int CLIENT_HORIZONTAL_SCALING {2};
inline constexpr int MIN_FOOD_IN_ARENA {6};
inline constexpr int FOOD_SPAWN_FROM_BODY_SEGMENT_PROBABILITY {3};
inline constexpr int SPEED_BOOST_PROBABILITY {80};
inline constexpr int SPEED_BOOST_DURATION_MS {8000};
inline constexpr float SPEED_BOOST_RATIO {1.5};
inline constexpr int BOOSTED_MOVEMENT_FREQUENCY_MS {static_cast<int>(MOVEMENT_FREQUENCY_MS * (1 / SPEED_BOOST_RATIO))};

inline constexpr int STATS_FREQUENCY_SECONDS {15};
inline constexpr int LOGGING_FLUSH_INTERVAL_SECONDS {3};
inline std::string LOGGING_FORMAT {"[%Y-%m-%d %H:%M:%S.%f] [{}] [%l] %v"};

namespace SnakeConstants {
    inline const std::string PLAYER_KEY_QUIT = "q";
    inline const std::string PLAYER_KEY_UP = "^";
    inline const std::string PLAYER_KEY_DOWN = "v";
    inline const std::string PLAYER_KEY_LEFT = "<";
    inline const std::string PLAYER_KEY_RIGHT = ">";
}

enum class Color {
    WHITE = 1,
    YELLOW = 2,
    RED = 3,
    GREEN = 4,
    CYAN = 5,
    MAGENTA = 6,
};