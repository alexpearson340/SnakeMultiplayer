#include "common/Constants.h"
#include "common/Log.h"
#include "common/MessageLogReader.h"
#include "snake_server/SnakeServer.h"

int main() {
    const std::string applicationName {"snake_server"};
    const std::string replayPath {applicationName + ".replay.jsonl"};
    initLogging(applicationName, false, true);

    // If there is a replay file, read the SERVER_CONFIG
    // message at the top of the file and use it to seed
    // the game engine
    std::optional<MessageLogReader> reader;
    std::optional<ProtocolMessage> header;
    if (std::filesystem::exists(replayPath)) {
        reader.emplace(replayPath);
        header = reader->next();
    }   

    SnakeServer server {initServerConfig(applicationName, header)};
    server.run();
    return 0;
}
