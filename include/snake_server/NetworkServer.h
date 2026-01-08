#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "engine/ProtocolMessage.h"

inline int MAX_EVENTS {10};

class NetworkServer {
public:
    NetworkServer(int);
    std::vector<ProtocolMessage> pollMessages();
    void broadcast(std::string_view msg);

private:
    void setNonBlocking(int fd);
    void registerFdWithEpoll(int fd);
    void acceptNewClient();
    std::vector<ProtocolMessage> receiveFromClient(int fd);

    int serverFd;
    int epollFd;
    int nextClientId;

    std::unordered_map<int, int> fdToClientIdMap;
};
