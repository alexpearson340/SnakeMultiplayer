#pragma once

#include <vector>
#include <string>
#include <utility>
#include "engine/Json.h"

namespace client {

    struct PlayerData {
        int clientId;
        char direction;
        std::string name;
        int score;
        std::vector<std::pair<int, int>> segments;
    };

    struct FoodData {
        int x;
        int y;
    };

    struct GameState {
        std::vector<PlayerData> players;
        std::vector<FoodData> food;
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
                segments
            });
        }

        // food
        std::vector<FoodData> food {};

        return {players, food};
    }

};
