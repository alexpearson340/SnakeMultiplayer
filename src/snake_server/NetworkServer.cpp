#include "snake_server/NetworkServer.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

NetworkServer::NetworkServer(int port)
    : serverFd {-1}
    , epollFd {-1}
    , nextClientId {1}
    , fdToClientIdMap {}
    , fdToBufferMap {} {

    // Create socket
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        throw std::runtime_error("Failed to create socket");
    }
    std::cout << "serverFd=" << serverFd << std::endl;

    // Allow address reuse
    int opt {1};
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(serverFd);
        throw std::runtime_error("Bind failed on port " + std::to_string(port));
    }

    // Listen
    if (listen(serverFd, SOMAXCONN) < 0) {
        close(serverFd);
        throw std::runtime_error("Listen failed");
    }

    // Make non-blocking
    setNonBlocking(serverFd);

    // Create epoll instance
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        close(serverFd);
        throw std::runtime_error("Failed to create epoll instance");
    }
    std::cout << "epollFd=" << epollFd << std::endl;

    registerFdWithEpoll(serverFd);

    std::cout << "Server listening on port " << port << std::endl;
}

std::vector<ProtocolMessage> NetworkServer::pollMessages() {
    std::vector<ProtocolMessage> messages;
    epoll_event events[MAX_EVENTS];
    int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, 0);
    std::cout << "numEvents=" << numEvents << std::endl;

    for (int i = 0; i < numEvents; i++) {
        if (events[i].data.fd == serverFd) {
            acceptNewClient();
        }
        else {
            for (ProtocolMessage pm : receiveFromClient(events[i].data.fd)) {
                messages.push_back(pm);
            }
        }
    }
    return messages;
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
    event.events = EPOLLIN | EPOLLET;  // Edge-triggered
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
    int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &addrLen);
    if (clientFd == -1) {
        std::cerr << "Accept new client connection failed" << std::endl;
    }
    else {
        setNonBlocking(clientFd);
        registerFdWithEpoll(clientFd);
    }

    // Make client socket non-blocking too
    setNonBlocking(clientFd);

    // Add client fd to epoll
    registerFdWithEpoll(clientFd);

    // Store mapping: fd -> clientId
    int clientId = nextClientId++;
    // TODO: Store in fdToClientId and clientIdToFd maps
    fdToClientIdMap[clientFd] = clientId;
    fdToBufferMap[clientFd] = "";

    std::cout << "Client " << clientId << " connected (fd: " << clientFd << ")" << std::endl;
}

std::vector<ProtocolMessage> NetworkServer::receiveFromClient(int fd) {
    char buffer[1024];
    int bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        // Connection closed or error
        std::cout << "Client disconnected (fd: " << fd << ")" << std::endl;

        ProtocolMessage msg;
        msg.messageType = MessageType::CLIENT_DISCONNECT;
        msg.clientId = fdToClientIdMap.at(fd);

        // Remove from epoll
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        fdToClientIdMap.erase(fd);
        fdToBufferMap.erase(fd);
        msg.messageType = MessageType::CLIENT_DISCONNECT;
        msg.message = std::to_string(msg.clientId);
        return std::vector<ProtocolMessage> {msg};
    }

    return parseReceivedPacket(fd, buffer, bytesRead);
}

std::vector<ProtocolMessage> NetworkServer::parseReceivedPacket(int fd, char* buffer, size_t size) {
    std::vector<ProtocolMessage> messages {};
    fdToBufferMap[fd] += std::string(buffer, size);
    size_t pos;
    while ((pos = fdToBufferMap.at(fd).find('\n')) != std::string::npos) {
        std::string msg {fdToBufferMap.at(fd).substr(0, pos)};
        ProtocolMessage pm {protocol::fromString(msg, fdToClientIdMap.at(fd))};
        messages.push_back(pm);
        fdToBufferMap.at(fd).erase(0, pos + 1);
    }
    return messages;
}

void NetworkServer::broadcast(std::string_view msg) {
    for (const auto & [fd, clientId] : fdToClientIdMap) {
        send(fd, msg.data(), msg.size(), 0);
    }
}
