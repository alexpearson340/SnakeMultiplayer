#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "engine/ClientMessage.h"

inline int MAX_EVENTS {10};

class NetworkServer {
public:
    NetworkServer(int);
    std::vector<ClientMessage> pollMessages();
    void broadcast(std::string_view msg);

private:
    void setNonBlocking(int fd);
    void registerFdWithEpoll(int fd);
    ClientMessage acceptNewClient();
    ClientMessage receiveFromClient(int fd);

    int serverFd;
    int epollFd;
    int nextClientId;

    std::unordered_map<int, int> fdToClientIdMap;
};
