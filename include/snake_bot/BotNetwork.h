#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include "snake_client/NetworkClient.h"

class BotNetwork {
public:
    BotNetwork(const std::string &, int);
    void joinBot(const std::string &);
    void sendToServer(const int, const std::string &) const;
    std::vector<ProtocolMessage> pollMessages();
    void destroyBot(const int);

private:
    void startBotNetwork();

    const std::string host;
    int port;
    int epollFd;
    std::unordered_map<int, int> clientIdToFdMap;
    std::unordered_map<int, std::unique_ptr<NetworkClient>> fdToNetworkClientMap;
};