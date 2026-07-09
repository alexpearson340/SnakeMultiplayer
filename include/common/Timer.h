#pragma once

#include <chrono>
#include <common/Constants.h>
#include <spdlog/spdlog.h>
#include <string>

class Timer {
public:
    Timer();
    void tick();
    void setTick(const int64_t);
    std::chrono::time_point<std::chrono::steady_clock> currentTick() const;
    int64_t currentTickAsNanos() const;

private:
    std::chrono::time_point<std::chrono::steady_clock> currentGameTick;
    std::chrono::time_point<std::chrono::steady_clock> previousStatTick;
    int engineLoopCounter;
};

inline Timer::Timer()
    : currentGameTick {std::chrono::steady_clock::now()},
      previousStatTick {std::chrono::steady_clock::now()},
      engineLoopCounter {0} {}

inline void Timer::tick() {
    engineLoopCounter++;
    currentGameTick = std::chrono::steady_clock::now();
    if (currentGameTick - previousStatTick > std::chrono::seconds(STATS_FREQUENCY_SECONDS)) {
        spdlog::info("IPS=" + std::to_string(engineLoopCounter / STATS_FREQUENCY_SECONDS));
        engineLoopCounter = 0;
        previousStatTick = currentGameTick;
    }
}

inline void Timer::setTick(const int64_t nanos) {
    currentGameTick = std::chrono::steady_clock::time_point {std::chrono::nanoseconds {nanos}};
}

inline std::chrono::time_point<std::chrono::steady_clock> Timer::currentTick() const {
    return currentGameTick;
};

inline int64_t Timer::currentTickAsNanos() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(currentGameTick.time_since_epoch()).count();
};