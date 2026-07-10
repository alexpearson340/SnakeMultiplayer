#pragma once

#include "common/ProtocolMessage.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class NetworkServer {
public:
    NetworkServer(int);
    std::vector<ProtocolMessage> pollMessages();
    void sendToClient(const int clientId, const ProtocolMessage & msg);
    void broadcast(const ProtocolMessage &);

private:
    void startServer(int);
    void setNonBlocking(int fd);
    void registerFdWithEpoll(int fd);
    void acceptNewClient();
    ProtocolMessage disconnectClient(const int);
    std::vector<ProtocolMessage> receiveFromClient(int fd);
    std::vector<ProtocolMessage> parseReceivedPacket(int fd, char * buffer, size_t size);
    void networkSend(const int, const std::string &);

    int serverFd;
    int epollFd;
    int nextClientId;

    std::unordered_map<int, int> fdToClientIdMap;
    std::unordered_map<int, int> clientIdToFdMap;
    std::unordered_map<int, std::string> fdToBufferMap;
    std::unordered_set<int> fdsToDisconnect;
};