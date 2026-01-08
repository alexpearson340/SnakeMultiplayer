#pragma once

#include <string>

enum class ClientMessageType {
    CLIENT_CONNECT,
    CLIENT_DISCONNECT,
    CLIENT_INPUT
};

struct ClientMessage {
    ClientMessageType messageType;
    int clientId;
    std::string data;
};
