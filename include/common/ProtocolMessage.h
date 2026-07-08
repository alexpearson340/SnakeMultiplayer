#pragma once

#include "common/Json.h"
#include <string>

enum class MessageType {
    CLIENT_CONNECT,    // first contact between client and server
    CLIENT_DISCONNECT, // end of contact between client and server
    CLIENT_JOIN,       // client to server, introduces the client
    SERVER_WELCOME,    // server acknowledgement of client join
    CLIENT_INPUT,      // client input of actions to server
    GAME_STATE         // server broadcast of game state out to clients
};

struct ProtocolMessage {
    MessageType messageType;
    std::string message;
    int clientId {-1};
    int64_t sequence {-1};
    int64_t transactTime {-1};
};

namespace protocol {

    inline std::string toString(const ProtocolMessage & msg) {
        json j {{"message_type", static_cast<int>(msg.messageType)},
                {"message", msg.message},
                {"client_id", msg.clientId},
                {"sequence", msg.sequence},
                {"transact_time", msg.transactTime}};
        return j.dump() + '\n';
    }

    inline ProtocolMessage fromString(const std::string_view str) {
        json j = json::parse(str);
        return {
            static_cast<MessageType>(j["message_type"]), j["message"], j["client_id"],
                static_cast<int64_t>(j["sequence"]), static_cast<int64_t>(j["transact_time"])
        };
    }

    inline ProtocolMessage fromString(const std::string_view str, int clientId) {
        ProtocolMessage pm {fromString(str)};
        pm.clientId = clientId;
        return pm;
    };

}; // namespace protocol
