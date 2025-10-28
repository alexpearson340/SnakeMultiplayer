#include "snake_server/SnakeServer.h"

int main() {
    SnakeServer server {10, 10};
    server.run();
    return 0;
}
