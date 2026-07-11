#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "common/ProtocolMessage.h"

class NetworkServer {
public:
    NetworkServer(int);
    std::vector<std::pair<int, Bytes>> pollMessages();
    std::vector<int> drainDisconnects();
    void sendToClient(const int clientId, const Bytes &);
    void broadcast(const Bytes &);


private:
    void startServer(int);
    void setNonBlocking(int fd);
    void registerFdWithEpoll(int fd);
    void acceptNewClient();
    void disconnectClient(const int);
    std::vector<Bytes> receiveFromClient(int fd);
    std::vector<Bytes> parseReceivedPacket(int fd, char * buffer, size_t size);
    void networkSend(const int, const Bytes &);

    int serverFd;
    int epollFd;
    int nextClientId;

    std::unordered_map<int, int> fdToClientIdMap;
    std::unordered_map<int, int> clientIdToFdMap;
    std::unordered_map<int, Bytes> fdToBufferMap;
    std::unordered_set<int> fdsToDisconnect;
};