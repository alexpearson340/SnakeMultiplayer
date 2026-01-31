#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "common/ProtocolMessage.h"

class NetworkServer {
public:
    NetworkServer(int);
    std::vector<ProtocolMessage> pollMessages();
    void sendToClient(int, std::string_view);
    void broadcast(std::string_view);

private:
    void startServer(int);
    void setNonBlocking(int fd);
    void registerFdWithEpoll(int fd);
    void acceptNewClient();
    std::vector<ProtocolMessage> receiveFromClient(int fd);
    std::vector<ProtocolMessage> parseReceivedPacket(int fd, char* buffer, size_t size);

    int serverFd;
    int epollFd;
    int nextClientId;

    std::unordered_map<int, int> fdToClientIdMap;
    std::unordered_map<int, int> clientIdToFdMap;
    std::unordered_map<int, std::string> fdToBufferMap;
};