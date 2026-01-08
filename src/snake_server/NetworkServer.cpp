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
    , fdToClientIdMap {} {

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

std::vector<ClientMessage> NetworkServer::pollMessages() {
    std::vector<ClientMessage> messages;
    epoll_event events[MAX_EVENTS];
    int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, 0);
    std::cout << "numEvents=" << numEvents << std::endl;

    for (int i = 0; i < numEvents; i++) {
        if (events[i].data.fd == serverFd) {
            ClientMessage newClientMessage {acceptNewClient()};
            if (newClientMessage.clientId != -1) {
                messages.push_back(newClientMessage);
            }
        } else {
            messages.push_back(receiveFromClient(events[i].data.fd));
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

ClientMessage NetworkServer::acceptNewClient() {
    ClientMessage msg;
    msg.messageType = ClientMessageType::CLIENT_CONNECT;
    msg.clientId = -1;
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    // Accept the connection - get a new fd for this client
    int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &addrLen);
    if (clientFd == -1) {
        std::cerr << "Accept failed" << std::endl;
        return msg;
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
    msg.clientId = clientId;
    // todo set the client's name in the message field
    msg.message = std::to_string(clientId);

    std::cout << "Client " << clientId << " connected (fd: " << clientFd << ")" << std::endl;
    return msg;
}

ClientMessage NetworkServer::receiveFromClient(int fd) {
    ClientMessage msg;
    msg.clientId = fdToClientIdMap.at(fd);

    char buffer[1024];
    int bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        // Connection closed or error
        std::cout << "Client disconnected (fd: " << fd << ")" << std::endl;

        // Remove from epoll
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        fdToClientIdMap.erase(fd);
        msg.messageType = ClientMessageType::CLIENT_DISCONNECT;
        msg.message = std::to_string(msg.clientId);
        return msg;
    }

    msg.messageType = ClientMessageType::CLIENT_INPUT;
    // Convert bytes to string
    buffer[bytesRead] = '\0';
    msg.message = std::string(buffer, bytesRead);
    if (!msg.message.empty() && msg.message.back() == '\n') {
        msg.message.pop_back();
    }
    std::cout << "Received from client (fd " << fd << "): " << msg.message << std::endl;
    return msg;
}

void NetworkServer::broadcast(std::string_view msg) {
    for (const auto & [fd, clientId] : fdToClientIdMap) {
        send(fd, msg.data(), msg.size(), 0);
    }
}
