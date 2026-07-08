#pragma once

#include "common/ProtocolMessage.h"
#include <fstream>
#include <stdexcept>
#include <string>

class MessageLogWriter {
public:
    explicit MessageLogWriter(const std::string & applicationName) : out {applicationName + ".jsonl", std::ios::out} {
        if (!out) {
            throw std::runtime_error("Failed to open log file " + applicationName + ".jsonl");
        }
    }

    void log(const ProtocolMessage & msg) {
        out << protocol::toString(msg);
        out.flush();
    }

private:
    std::ofstream out;
};