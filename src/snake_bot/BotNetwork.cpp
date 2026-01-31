#include <memory>
#include <string>
#include <sys/epoll.h>
#include <stdexcept>
#include "common/Log.h"
#include "snake_bot/BotNetwork.h"

BotNetwork::BotNetwork(const std::string & host, int port)
    : host {host}
    , port {port}
    , epollFd {-1}
    , clientIdToFdMap {}
    , fdToNetworkClientMap {} {
    startBotNetwork();
}

void BotNetwork::startBotNetwork() {
    // create epoll instance
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        throw std::runtime_error("Failed to create epoll instance");
    }
    spdlog::info("epollFd=" + std::to_string(epollFd));
}

void BotNetwork::joinBot(const std::string & clientJoinMessage) {
    std::unique_ptr<NetworkClient> newNetworkClient {std::make_unique<NetworkClient>(host, port)};
    int fd {newNetworkClient->getServerFd()};

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
        close(epollFd);
        close(fd);
        throw std::runtime_error("Failed to add fd to epoll");
    }
    fdToNetworkClientMap[fd] = std::move(newNetworkClient);
    fdToNetworkClientMap[fd]->sendToServer(clientJoinMessage);
}

void BotNetwork::sendToServer(const int clientId, const std::string & msg) const {
    fdToNetworkClientMap.at(clientIdToFdMap.at(clientId))->sendToServer(msg);
}

std::vector<ProtocolMessage> BotNetwork::pollMessages() {
    std::vector<ProtocolMessage> messages;
    epoll_event events[MAX_EVENTS];
    int numEvents {epoll_wait(epollFd, events, MAX_EVENTS, EPOLL_BLOCKING_TIMEOUT_MS)};

    for (int i = 0; i < numEvents; i++) {
        for (ProtocolMessage pm : fdToNetworkClientMap.at(events[i].data.fd)->receiveFromServer()) {
            if (pm.messageType == MessageType::SERVER_WELCOME) {
                if (clientIdToFdMap.contains(pm.clientId)) {
                    throw std::runtime_error("How can clientId=" + std::to_string(pm.clientId) + " already be known on SERVER_WELCOME?");
                }
                clientIdToFdMap[pm.clientId] = events[i].data.fd;
            }
            messages.push_back(pm);
        }
    }
    return messages;
}

void BotNetwork::destroyBot(const int clientId) {
    spdlog::info("Destroying clientId=" + std::to_string(clientId));
    int fd {clientIdToFdMap.at(clientId)};
    // Remove from epoll
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    fdToNetworkClientMap.erase(fd);
    clientIdToFdMap.erase(clientId);
}
