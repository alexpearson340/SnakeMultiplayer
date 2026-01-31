#pragma once

#include <chrono>
#include <spdlog/spdlog.h>
#include <string>
#include <common/Constants.h>

class Timer {
public:
    Timer();
    void tick();
    std::chrono::time_point<std::chrono::steady_clock> currentTick() {return currentGameTick;};

private:
    std::chrono::time_point<std::chrono::steady_clock> currentGameTick;
    std::chrono::time_point<std::chrono::steady_clock> previousStatTick;
    int engineLoopCounter;
};

inline Timer::Timer()
    : currentGameTick {std::chrono::steady_clock::now()}
    , previousStatTick {std::chrono::steady_clock::now()}
    , engineLoopCounter {0} {
}

inline void Timer::tick() {
    engineLoopCounter++;
    currentGameTick = std::chrono::steady_clock::now();
    if (currentGameTick - previousStatTick > std::chrono::seconds(STATS_FREQUENCY_SECONDS)) {
        spdlog::info("IPS=" + std::to_string(engineLoopCounter / STATS_FREQUENCY_SECONDS));
        engineLoopCounter = 0;
        previousStatTick = currentGameTick;
    }
}