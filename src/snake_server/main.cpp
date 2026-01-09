#include "common/Constants.h"
#include "snake_server/SnakeServer.h"

int main() {
    SnakeServer server {ARENA_WIDTH, ARENA_HEIGHT};
    server.run();
    return 0;
}
