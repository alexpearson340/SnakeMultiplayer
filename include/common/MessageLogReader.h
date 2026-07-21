#pragma once

#include "common/ProtocolMessage.h"

#include <cassert>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>

class MessageLogReader {
public:
    explicit MessageLogReader(const std::string & fileName) : in {fileName, std::ios::in}, currentTransactTime {0} {
        if (!in) {
            throw std::runtime_error("Failed to open log file " + fileName);
        }
    }

    std::optional<ProtocolMessage> first() {
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) {
                continue;
            }
            return jsonprotocol::fromString(line);
        }
        return std::nullopt;
    }

    // return messages all of the same transactTime
    std::vector<ProtocolMessage> nextBatch() {
        std::vector<ProtocolMessage> output {};
        std::string line;
        if (outputBuffer.has_value()) {
            output.push_back(std::move(outputBuffer.value()));
            outputBuffer.reset();
            currentTransactTime = output.front().transactTime;
        }

        while (std::getline(in, line)) {
            if (line.empty()) {
                continue;
            }
            ProtocolMessage pm {jsonprotocol::fromString(line)};
            if (currentTransactTime == 0) {
                output.push_back(pm);
                currentTransactTime = pm.transactTime;
            }
            else if (pm.transactTime == currentTransactTime) {
                output.push_back(pm);
            }
            else if (pm.transactTime > currentTransactTime) {
                outputBuffer.emplace(pm);
                currentTransactTime = pm.transactTime;
                break;
            }
            else {
                throw std::logic_error("logic error in MessageLogReader::nextBatch loop");
            }
        }
        return output;
    }

private:
    std::ifstream in;
    int64_t currentTransactTime;
    std::optional<ProtocolMessage> outputBuffer {};
};
