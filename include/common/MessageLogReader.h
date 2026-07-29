#pragma once

#include "common/ProtocolMessage.h"

#include <cassert>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>

class MessageLogReader {
public:
    explicit MessageLogReader(const std::string & fileName)
        : in {fileName, std::ios::in | std::ios::binary}, currentTransactTime {0} {
        if (!in) {
            throw std::runtime_error("Failed to open log file " + fileName);
        }
    }

    std::optional<ProtocolMessage> first() {
        uint32_t len;
        std::string msg;
        if (in.read(reinterpret_cast<char *>(&len), sizeof(len))) {
            msg.resize(len);
            if (!in.read(msg.data(), len)) {
                throw std::runtime_error("truncated record in message log");
            }
            return jsonprotocol::fromString(msg);
        }
        return std::nullopt;
    }

    // return messages all of the same transactTime
    std::vector<ProtocolMessage> nextBatch() {
        std::vector<ProtocolMessage> output {};
        if (outputBuffer.has_value()) {
            output.push_back(std::move(outputBuffer.value()));
            outputBuffer.reset();
            currentTransactTime = output.front().transactTime;
        }

        uint32_t len;
        std::string msg;
        while (in.read(reinterpret_cast<char *>(&len), sizeof(len))) {
            msg.resize(len);
            if (!in.read(msg.data(), len)) {
                throw std::runtime_error("truncated record in message log");
            }
            ProtocolMessage pm {jsonprotocol::fromString(msg)};
            if (currentTransactTime == 0) {
                output.push_back(pm);
                currentTransactTime = pm.transactTime;
            } else if (pm.transactTime == currentTransactTime) {
                output.push_back(pm);
            } else if (pm.transactTime > currentTransactTime) {
                outputBuffer.emplace(pm);
                currentTransactTime = pm.transactTime;
                break;
            } else {
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
