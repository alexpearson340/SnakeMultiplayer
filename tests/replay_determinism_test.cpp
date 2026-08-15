#include "common/Protocol.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

    std::vector<std::string> readMessages(const std::filesystem::path & path) {
        std::ifstream in {path, std::ios::binary};
        std::vector<std::string> messages;

        uint32_t len;
        std::string msg;
        while (in.read(reinterpret_cast<char *>(&len), sizeof(len))) {
            msg.resize(len);
            if (!in.read(msg.data(), len)) {
                throw std::runtime_error("truncated record in " + path.string());
            }
            messages.push_back(msg);
        }
        return messages;
    }

} // namespace

// Replaying a recording should reproduce the recording byte-for-byte. The one
// exception is record 1 (SERVER_CONFIG): its transact_time is wall clock
TEST(ReplayDeterminism, ReproducesRecording) {
    const std::filesystem::path fixture {std::filesystem::path {FIXTURE_DIR} / "test_replay.bin"};
    const std::filesystem::path workDir {std::filesystem::temp_directory_path() / "snake_replay_determinism_test"};

    std::filesystem::remove_all(workDir);
    std::filesystem::create_directories(workDir);

    const std::string cmd {"cd " + workDir.string() + " && SNAKE_REPLAY=" + fixture.string() + " " + SNAKE_SERVER_BIN +
                           " > /dev/null 2>&1"};
    ASSERT_EQ(std::system(cmd.c_str()), 0) << "snake_server replay run failed";

    const std::vector<std::string> expected {readMessages(fixture)};
    const std::vector<std::string> actual {readMessages(workDir / "snake_server.bin")};

    ASSERT_EQ(actual.size(), expected.size()) << "output record count differs from the fixture";

    for (std::size_t i {1}; i < expected.size(); ++i) {
        EXPECT_EQ(actual[i], expected[i]) << "divergence at record " << (i + 1);
    }

    const protocol::MessageVariant actualLast {protocol::deserialise(actual.back())};
    const protocol::Header & hdr {protocol::header(actualLast)};
    EXPECT_EQ(hdr.messageType, protocol::MessageType::GAME_STATE);
    EXPECT_EQ(hdr.clientId, -1);
    EXPECT_EQ(hdr.sequence, 163167);
    EXPECT_EQ(hdr.transactTime, 13887458496418125);

    const protocol::GameState & gs {std::get<protocol::GameState>(actualLast)};
    EXPECT_EQ(gs.highScore, 37);
    EXPECT_STREQ(gs.highScoreUsername, "bot");
    EXPECT_EQ(gs.food.size(), 6u);
    EXPECT_EQ(gs.speedBoosts.size(), 1u);
    ASSERT_EQ(gs.players.size(), 20u);

    // Anchor on one concrete player: client_id 4, the six-segment snake.
    const auto it {std::find_if(gs.players.begin(), gs.players.end(),
                                [](const protocol::GameState::Player & p) { return p.clientId == 4; })};
    ASSERT_NE(it, gs.players.end());
    EXPECT_EQ(it->color, 6);
    EXPECT_EQ(it->direction, '<');
    EXPECT_EQ(it->score, 6);
    EXPECT_STREQ(it->username, "bot");
    EXPECT_EQ(it->segments, (std::vector<protocol::GameState::Player::Segment> {
                                {6, 31}, {7, 31}, {7, 30}, {7, 29}, {7, 28}, {7, 27}}));
}
