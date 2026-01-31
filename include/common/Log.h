#include <string>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <common/Constants.h>

inline void initLogging(std::string applicationName, bool fileLogging, bool stdoutLogging) {
    std::vector<spdlog::sink_ptr> sinks;
    if (fileLogging) {
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(applicationName + ".log"));
    }
    if (stdoutLogging) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    }
    auto logger = std::make_shared<spdlog::logger>(applicationName, sinks.begin(), sinks.end());

    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_every(std::chrono::seconds(LOGGING_FLUSH_INTERVAL_SECONDS));
    spdlog::set_pattern(fmt::format(fmt::runtime(LOGGING_FORMAT), applicationName));
    spdlog::info("Starting " + applicationName);
    spdlog::default_logger()->flush();
}
