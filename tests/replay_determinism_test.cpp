#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

    std::vector<std::string> readLines(const std::filesystem::path & path) {
        std::ifstream in {path};
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line)) {
            lines.push_back(line);
        }
        return lines;
    }

} // namespace

// Replaying a recording should reproduce the recording byte-for-byte. The one
// exception is line 1 (SERVER_CONFIG): its transact_time is wall clock
TEST(ReplayDeterminism, ReproducesRecording) {
    const std::filesystem::path fixture {std::filesystem::path {FIXTURE_DIR} / "test_replay.jsonl"};
    const std::filesystem::path workDir {std::filesystem::temp_directory_path() / "snake_replay_determinism_test"};

    std::filesystem::remove_all(workDir);
    std::filesystem::create_directories(workDir);

    const std::string cmd {"cd " + workDir.string() + " && SNAKE_REPLAY=" + fixture.string() + " "
                           + SNAKE_SERVER_BIN + " > /dev/null 2>&1"};
    ASSERT_EQ(std::system(cmd.c_str()), 0) << "snake_server replay run failed";

    const std::vector<std::string> expected {readLines(fixture)};
    const std::vector<std::string> actual {readLines(workDir / "snake_server.jsonl")};

    ASSERT_EQ(actual.size(), expected.size()) << "output line count differs from the fixture";

    for (std::size_t i {1}; i < expected.size(); ++i) {
        EXPECT_EQ(actual[i], expected[i]) << "divergence at line " << (i + 1);
    }
}
