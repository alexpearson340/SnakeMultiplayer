#include "common/Constants.h"
#include "common/Log.h"
#include "snake_bot/SnakeBot.h"

int main() {
    initLogging("snake_bot", false, true);
    SnakeBot bot {ARENA_WIDTH, ARENA_HEIGHT};
    bot.run();
    return 0;
}