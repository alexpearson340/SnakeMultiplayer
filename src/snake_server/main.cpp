#include "common/Constants.h"
#include "common/Log.h"
#include "snake_server/SnakeServer.h"

int main() {
    initLogging("snake_server");
    SnakeServer server {ARENA_WIDTH, ARENA_HEIGHT};
    server.run();
    return 0;
}
