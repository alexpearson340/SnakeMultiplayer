#include "common/Log.h"
#include "common/MessageLogReader.h"
#include "snake_server/SnakeServer.h"

#include <cstdlib>
#include <optional>
#include <string>

int main() {
    const std::string applicationName {"snake_server"};
    initLogging(applicationName, false, true);

    // Replay mode is selected by the SNAKE_REPLAY env var, whose value
    // is the replay file path. If it is unset, run in normal mode
    std::optional<MessageLogReader> reader;
    std::optional<ProtocolMessage> header;
    if (const char * replayPath = std::getenv("SNAKE_REPLAY"); replayPath != nullptr && replayPath[0] != '\0') {
        reader.emplace(replayPath);
        header = reader->first();
    }

    SnakeServer server {initServerConfig(applicationName, header), std::move(reader)};
    server.run();
    return 0;
}
