#include "snake_server/NetworkServer.h"
#include "common/Constants.h"
#include "common/Log.h"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

NetworkServer::NetworkServer(int port)
    : serverFd {-1}, epollFd {-1}, nextClientId {1}, fdToClientIdMap {}, clientIdToFdMap {}, fdToBufferMap {}, fdsToDisconnect {} {
    startServer(port);
}

void NetworkServer::startServer(int port) {
    // Create socket
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        throw std::runtime_error("Failed to create socket");
    }
    spdlog::info("serverFd=" + std::to_string(serverFd));

    // Allow address reuse
    int opt {1};
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0) {
        close(serverFd);
        throw std::runtime_error("Bind failed on port " + std::to_string(port));
    }

    // Listen
    if (listen(serverFd, SOMAXCONN) < 0) {
        close(serverFd);
        throw std::runtime_error("Listen failed");
    }
    setNonBlocking(serverFd);

    // Create epoll instance
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        close(serverFd);
        throw std::runtime_error("Failed to create epoll instance");
    }
    spdlog::info("epollFd=" + std::to_string(epollFd));

    // Add the server socket to epoll
    registerFdWithEpoll(serverFd);
    spdlog::info("Server listening on port " + std::to_string(port));
}

std::vector<std::pair<int, Bytes>> NetworkServer::pollMessages() {
    assert(fdsToDisconnect.empty() && "fds awaiting destruction at start of fresh pollMessages loop");

    std::vector<std::pair<int, Bytes>> messages;
    epoll_event events[MAX_EVENTS];
    int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, EPOLL_BLOCKING_TIMEOUT_MS);

    for (int i = 0; i < numEvents; i++) {
        int fd {events[i].data.fd};
        if (fd == serverFd) {
            acceptNewClient();
        } else {
            for (Bytes bytes : receiveFromClient(fd)) {
                messages.push_back({fdToClientIdMap.at(fd), bytes});
            }
        }
    }
    return messages;
}

std::vector<int> NetworkServer::drainDisconnects() {
    std::vector<int> clientIds;
    for (int fd : fdsToDisconnect) {
        if (fdToClientIdMap.contains(fd)) {
            clientIds.push_back(fdToClientIdMap.at(fd));
            disconnectClient(fd);
        }
    }
    fdsToDisconnect.clear();
    return clientIds;
}

void NetworkServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("fcntl F_GETFL failed");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("fcntl F_SETFL failed");
    }
}

void NetworkServer::registerFdWithEpoll(int fd) {
    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
        close(epollFd);
        close(fd);
        throw std::runtime_error("Failed to add fd to epoll");
    }
}

void NetworkServer::acceptNewClient() {
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    // Accept the connection - get a new fd for this client
    int clientFd = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr), &addrLen);
    if (clientFd == -1) {
        spdlog::error("Accept failed");
    } else {
        setNonBlocking(clientFd);
        registerFdWithEpoll(clientFd);

        int clientId = nextClientId++;
        fdToClientIdMap[clientFd] = clientId;
        clientIdToFdMap[clientId] = clientFd;
        fdToBufferMap[clientFd] = "";

        spdlog::info("Client " + std::to_string(clientId) + " connected (fd: " + std::to_string(clientFd) + ")");
    }
}

void NetworkServer::disconnectClient(const int fd) {
    spdlog::info("Disconnecting client fd={}", fd);
    assert(fdToClientIdMap.contains(fd) && "disconnectClient: fd already gone");

    // Remove from epoll
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    clientIdToFdMap.erase(fdToClientIdMap.at(fd));
    fdToClientIdMap.erase(fd);
    fdToBufferMap.erase(fd);
}

std::vector<Bytes> NetworkServer::receiveFromClient(int fd) {
    char buffer[1024];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return {};
        } else {
            spdlog::warn("Retrieved errno {} on recv from fd={}", errno, fd);
            fdsToDisconnect.insert(fd);
            return {};
        };
    } else if (bytesRead == 0) {
        fdsToDisconnect.insert(fd);
        return {};
    }

    return parseReceivedPacket(fd, buffer, static_cast<size_t>(bytesRead));
}

std::vector<Bytes> NetworkServer::parseReceivedPacket(int fd, char * buffer, size_t size) {
    std::vector<Bytes> frames {};
    fdToBufferMap[fd] += Bytes(buffer, size);
    size_t pos;
    while ((pos = fdToBufferMap.at(fd).find('\n')) != Bytes::npos) {
        frames.push_back(fdToBufferMap.at(fd).substr(0, pos));
        fdToBufferMap.at(fd).erase(0, pos + 1);
    }
    return frames;
}

void NetworkServer::sendToClient(const int clientId, const Bytes & bytes) {
    networkSend(clientIdToFdMap.at(clientId), bytes); 
}

void NetworkServer::broadcast(const Bytes & bytes) {
    for (const auto & [fd, clientId] : fdToClientIdMap) {
        networkSend(fd, bytes);
    }
}

void NetworkServer::networkSend(const int fd, const Bytes & bytes) {
    size_t size {bytes.size()};
    ssize_t sent {send(fd, bytes.data(), size, 0)};
    
    // treat partial sends, and all error codes
    // as a client disconnect. Buffer + retry on
    // partial sends and EAGAIN is todo
    if (0 <= sent && static_cast<size_t>(sent) < size) {
        spdlog::warn("Partial send for fd={}, tried to send {} bytes, actually sent {}, disconnecting the client", fd, size, sent);
        fdsToDisconnect.insert(fd);
    }
    else if (sent == -1) {
        spdlog::warn("Error receieved {} on send for fd={}, disconnecting the client", errno, fd);
        fdsToDisconnect.insert(fd);
    }
}
