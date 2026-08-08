#pragma once

#include "common/Protocol.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
        std::unordered_map<int, PlayerData> players;
        std::vector<FoodData> food;
        std::vector<SpeedBoostsData> speedBoosts;
        std::pair<std::string, int> serverHighScore;
    };

    inline GameState fromProtocol(const protocol::GameState & msg) {
        // players
        std::unordered_map<int, PlayerData> players {};
        for (auto & p : msg.players) {
            std::vector<std::pair<int, int>> segments {};
            for (auto & s : p.segments) {
                segments.push_back({s.first, s.second});
            }
            players[p.clientId] = {p.clientId, p.direction, p.username, p.score, p.color, segments};
        }

        // food
        std::vector<FoodData> food {};
        for (auto & f : msg.food) {
            food.push_back({f.x, f.y, f.icon, f.color});
        }

        // speed boosts
        std::vector<SpeedBoostsData> speedBoosts {};
        for (auto & s : msg.speedBoosts) {
            speedBoosts.push_back({s.x, s.y, s.icon, s.color});
        }

        std::pair<std::string, int> serverHighScore {msg.highScoreUsername, msg.highScore};

        return {players, food, speedBoosts, serverHighScore};
    }

}; // namespace client
