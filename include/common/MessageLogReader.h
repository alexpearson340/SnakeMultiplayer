#pragma once

#include "common/ProtocolMessage.h"

#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>

class MessageLogReader {
public:
    explicit MessageLogReader(const std::string & fileName) : in {fileName, std::ios::in} {
        if (!in) {
            throw std::runtime_error("Failed to open log file " + fileName);
        }
    }

    std::optional<ProtocolMessage> next() {
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) {
                continue;
            }
            return protocol::fromString(line);
        }
        return std::nullopt;
    }

private:
    std::ifstream in;
};
