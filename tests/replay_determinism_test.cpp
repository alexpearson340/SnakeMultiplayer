#include "common/ProtocolMessage.h"

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

    const ProtocolMessage actualLast {protocol::fromString(actual.back())};
    EXPECT_EQ(actualLast.messageType, MessageType::GAME_STATE);
    EXPECT_EQ(actualLast.clientId, -1);
    EXPECT_EQ(actualLast.sequence, 12224);
    EXPECT_EQ(actualLast.transactTime, 10888288655790333);
    EXPECT_EQ(actualLast.message,
        R"json({"food":[{"color":3,"icon":"@","x":2,"y":18},{"color":3,"icon":"@","x":1,"y":20},{"color":1,"icon":"@","x":24,"y":8},{"color":1,"icon":"@","x":35,"y":39},{"color":1,"icon":"@","x":2,"y":2},{"color":1,"icon":"@","x":27,"y":30},{"color":1,"icon":"@","x":5,"y":39},{"color":1,"icon":"@","x":37,"y":35}],"players":[{"client_id":11,"color":3,"direction":">","name":"bot","score":1,"segments":[[26,22]]},{"client_id":12,"color":4,"direction":">","name":"bot","score":1,"segments":[[9,12]]},{"client_id":2,"color":4,"direction":"^","name":"bot","score":2,"segments":[[26,30],[26,31]]},{"client_id":13,"color":5,"direction":"^","name":"bot","score":3,"segments":[[18,15],[18,16],[18,17]]},{"client_id":7,"color":4,"direction":"v","name":"bot","score":2,"segments":[[20,3],[20,2]]},{"client_id":9,"color":6,"direction":"<","name":"bot","score":6,"segments":[[7,24],[8,24],[8,23],[8,22],[8,21],[8,20]]},{"client_id":19,"color":6,"direction":">","name":"bot","score":8,"segments":[[35,35],[34,35],[33,35],[33,34],[33,33],[33,32],[33,31],[33,30]]},{"client_id":17,"color":4,"direction":"v","name":"bot","score":4,"segments":[[37,28],[37,27],[37,26],[37,25]]},{"client_id":1,"color":3,"direction":"<","name":"bot","score":5,"segments":[[11,25],[12,25],[12,24],[12,23],[12,22]]},{"client_id":20,"color":2,"direction":">","name":"bot","score":6,"segments":[[2,26],[1,26],[1,25],[1,24],[1,23],[2,23]]},{"client_id":3,"color":5,"direction":"<","name":"bot","score":6,"segments":[[6,11],[7,11],[7,12],[7,13],[7,14],[6,14]]},{"client_id":16,"color":3,"direction":"<","name":"bot","score":5,"segments":[[38,35],[39,35],[39,36],[40,36],[40,35]]},{"client_id":18,"color":5,"direction":"v","name":"bot","score":4,"segments":[[32,6],[32,5],[33,5],[33,6]]},{"client_id":14,"color":6,"direction":"<","name":"bot","score":6,"segments":[[5,2],[6,2],[7,2],[8,2],[8,3],[7,3]]},{"client_id":8,"color":5,"direction":">","name":"bot","score":8,"segments":[[24,2],[23,2],[23,3],[23,4],[23,5],[23,6],[23,7],[23,8]]},{"client_id":6,"color":3,"direction":"^","name":"bot","score":7,"segments":[[13,10],[13,11],[13,12],[13,13],[13,14],[13,15],[14,15]]},{"client_id":5,"color":2,"direction":"^","name":"bot","score":8,"segments":[[12,1],[12,2],[12,3],[11,3],[11,4],[11,5],[11,6],[11,7]]},{"client_id":4,"color":6,"direction":"v","name":"bot","score":7,"segments":[[25,4],[25,3],[26,3],[26,4],[27,4],[28,4],[28,5]]},{"client_id":15,"color":2,"direction":"v","name":"bot","score":8,"segments":[[34,30],[34,29],[34,28],[34,27],[34,26],[34,25],[34,24],[34,23]]},{"client_id":10,"color":2,"direction":"^","name":"bot","score":6,"segments":[[16,2],[16,3],[16,4],[15,4],[14,4],[13,4]]}],"server_high_score":["bot",8],"speed_boosts":[]})json");
}
