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

    std::optional<protocol::MessageVariant> first() {
        uint32_t len;
        std::string msg;
        if (in.read(reinterpret_cast<char *>(&len), sizeof(len))) {
            msg.resize(len);
            if (!in.read(msg.data(), len)) {
                throw std::runtime_error("truncated record in message log");
            }
            return protocol::deserialise(msg);
        }
        return std::nullopt;
    }

    // return messages all of the same transactTime
    std::vector<protocol::MessageVariant> nextBatch() {
        std::vector<protocol::MessageVariant> output {};
        if (outputBuffer.has_value()) {
            output.push_back(std::move(outputBuffer.value()));
            outputBuffer.reset();
            currentTransactTime = protocol::header(output.front()).transactTime;
        }

        uint32_t len;
        std::string msg;
        while (in.read(reinterpret_cast<char *>(&len), sizeof(len))) {
            msg.resize(len);
            if (!in.read(msg.data(), len)) {
                throw std::runtime_error("truncated record in message log");
            }
            protocol::MessageVariant pm {protocol::deserialise(msg)};
            if (currentTransactTime == 0) {
                output.push_back(pm);
                currentTransactTime = protocol::header(pm).transactTime;
            } else if (protocol::header(pm).transactTime == currentTransactTime) {
                output.push_back(pm);
            } else if (protocol::header(pm).transactTime > currentTransactTime) {
                outputBuffer.emplace(pm);
                currentTransactTime = protocol::header(pm).transactTime;
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
    std::optional<protocol::MessageVariant> outputBuffer {};
};
