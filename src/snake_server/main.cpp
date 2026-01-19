#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "common/Constants.h"
#include "snake_server/SnakeServer.h"

int main() {
    auto logger = spdlog::basic_logger_mt("snake_server", "snake_server.log");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_every(std::chrono::seconds(LOGGING_FLUSH_INTERVAL_SECONDS));
    spdlog::set_pattern(LOGGING_FORMAT);
    
    SnakeServer server {ARENA_WIDTH, ARENA_HEIGHT};
    server.run();
    return 0;
}
