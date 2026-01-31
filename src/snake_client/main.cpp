#include "common/Constants.h"
#include "common/Log.h"
#include "snake_client/SnakeClient.h"

int main() {
    initLogging("snake_client", true, false);
    SnakeClient client {ARENA_WIDTH, ARENA_HEIGHT};
    client.run();
    return 0;
}
