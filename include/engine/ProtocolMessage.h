#pragma once

#include <string>
#include "engine/Json.h"

enum class MessageType {
    CLIENT_CONNECT,
    CLIENT_DISCONNECT,
    CLIENT_INPUT
};

struct ProtocolMessage {
    MessageType messageType;
    int clientId;
    std::string message;
};

inline json toJson(const ProtocolMessage & msg) {
    return {
        {"message_type", static_cast<int>(msg.messageType)},
        {"client_id", msg.clientId},
        {"message", msg.message}
    };
};

inline ProtocolMessage fromJson(const json & j) {
    return {
        static_cast<MessageType>(j["message_type"]),
        j["clientId"],
        j["message"]
    };
};
