#include <string>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <common/Constants.h>

inline void initLogging(std::string applicationName) {
    auto logger = spdlog::basic_logger_mt(applicationName, applicationName + ".log");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_every(std::chrono::seconds(LOGGING_FLUSH_INTERVAL_SECONDS));
    spdlog::set_pattern(fmt::format(fmt::runtime(LOGGING_FORMAT), applicationName));
    spdlog::info("Starting " + applicationName);
    spdlog::default_logger()->flush();
}
