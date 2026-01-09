#include "common/Constants.h"
#include "snake_client/SnakeClient.h"

int main() {
    SnakeClient client {ARENA_WIDTH, ARENA_HEIGHT};
    client.run();
    return 0;
}
