#pragma once

#include "common/ProtocolMessage.h"
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

class MessageLogWriter {
public:
    explicit MessageLogWriter(const std::string & applicationName)
        : out {applicationName + ".bin", std::ios::out | std::ios::binary} {
        if (!out) {
            throw std::runtime_error("Failed to open log file " + applicationName + ".bin");
        }
    }

    void log(const std::string & msg) {
        uint32_t len {static_cast<uint32_t>(msg.size())};
        out.write(reinterpret_cast<const char *>(&len), sizeof(len));
        out.write(msg.data(), len);
    }

private:
    std::ofstream out;
};