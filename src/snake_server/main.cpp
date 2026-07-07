#include "common/Constants.h"
#include "common/Log.h"
#include "snake_server/SnakeServer.h"

int main() {
    const std::string applicationName {"snake_server"};
    initLogging(applicationName, false, true);
    SnakeServer server {ARENA_WIDTH, ARENA_HEIGHT, applicationName};
    server.run();
    return 0;
}
