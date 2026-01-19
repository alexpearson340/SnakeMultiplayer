#include "common/Constants.h"
#include "snake_bot/SnakeBot.h"

int main() {
    SnakeBot bot {ARENA_WIDTH, ARENA_HEIGHT};
    bot.run();
    return 0;
}