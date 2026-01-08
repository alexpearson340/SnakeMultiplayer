#pragma once

#include <string>
#include "engine/Json.h"

enum class ClientMessageType {
    CLIENT_CONNECT,
    CLIENT_DISCONNECT,
    CLIENT_INPUT
};

struct ClientMessage {
    ClientMessageType messageType;
    int clientId;
    std::string message;
};

inline json toJson(const ClientMessage & msg) {
    return {
        {"message_type", static_cast<int>(msg.messageType)},
        {"client_id", msg.clientId},
        {"message", msg.message}
    };
};

inline ClientMessage fromJson(const json & j) {
    return {
        static_cast<ClientMessageType>(j["message_type"]),
        j["clientId"],
        j["message"]
    };
};
