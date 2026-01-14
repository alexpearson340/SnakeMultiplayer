#pragma once

#include <vector>
#include <string>
#include <utility>
#include "common/Json.h"

namespace client {

    struct PlayerData {
        int clientId;
        char direction;
        std::string name;
        int score;
        int color;
        std::vector<std::pair<int, int>> segments;
    };

    struct FoodData {
        int x;
        int y;
        char icon;
        int color;
    };

    struct SpeedBoostsData {
        int x;
        int y;
        char icon;
        int color;
    };

    struct GameState {
        std::vector<PlayerData> players;
        std::vector<FoodData> food;
        std::vector<SpeedBoostsData> speedBoosts;
        std::pair<std::string, int> serverHighScore;
    };

    inline GameState parseGameState(const std::string & jsonStr) {
        json j = json::parse(jsonStr);

        // players
        std::vector<PlayerData> players {};
        for (auto & p : j["players"]) {
            std::vector<std::pair<int, int>> segments {};
            for (auto & s : p["segments"]) {
                segments.push_back({s[0], s[1]});
            }
            players.push_back({
                p["client_id"],
                p["direction"].get<std::string>()[0],
                p["name"],
                p["score"],
                p["color"],
                segments
            });
        }

        // food
        std::vector<FoodData> food {};
        for (auto & f: j["food"]) {
            food.push_back({
                f["x"],
                f["y"],
                f["icon"].get<std::string>()[0],
                f["color"]
            });
        }

        // speed boosts
        std::vector<SpeedBoostsData> speedBoosts {};
        for (auto & s: j["speed_boosts"]) {
            speedBoosts.push_back({
                s["x"],
                s["y"],
                s["icon"].get<std::string>()[0],
                s["color"]
            });
        }

        std::pair<std::string, int> serverHighScore {j["server_high_score"][0], j["server_high_score"][1]};

        return {players, food, speedBoosts, serverHighScore};
    }

};
