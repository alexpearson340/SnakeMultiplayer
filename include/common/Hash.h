#pragma once

#include <functional>
#include <utility>

struct PairHash {
    std::size_t operator()(const std::pair<int, int> & p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};
