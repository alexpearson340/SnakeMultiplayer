#pragma once

#include <string>
#include "engine/Json.h"

enum class MessageType {
    CLIENT_CONNECT,        // first contact between client and server
    CLIENT_DISCONNECT,     // end of contact between client and server
    CLIENT_JOIN,           // client to server, introduces the client
    SERVER_WELCOME,        // server acknowledgement of client join
    CLIENT_INPUT,          // client input of actions to server
    GAME_STATE             // server broadcast of game state out to clients
};

struct ProtocolMessage {
    MessageType messageType;
    int clientId;
    std::string message;
};

namespace protocol {

    inline std::string toString(const ProtocolMessage & msg) {
        json j {
            {"message_type", static_cast<int>(msg.messageType)},
            {"client_id", msg.clientId},
            {"message", msg.message}
        };
        return j.dump();
    }

    inline ProtocolMessage fromString(const std::string_view str) {
        json j = json::parse(str);
        return {
            static_cast<MessageType>(j["message_type"]),
            j["client_id"],
            j["message"]
        };
    }

};
