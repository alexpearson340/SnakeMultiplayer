#include "engine/NetworkLayer.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

NetworkLayer::NetworkLayer(int port)
    : serverFd {-1}
    , epollFd {-1}
    , nextClientId {1} {

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

    // Add server socket to epoll
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;  // Edge-triggered
    event.data.fd = serverFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event) == -1) {
        close(epollFd);
        close(serverFd);
        throw std::runtime_error("Failed to add server socket to epoll");
    }

    std::cout << "Server listening on port " << port << std::endl;
}

std::vector<ClientMessage> NetworkLayer::pollMessages() {
    std::vector<ClientMessage> messages;
    epoll_event events[MAX_EVENTS];
    int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, 0);

    for (int i = 0; i < numEvents; i++) {
        if (events[i].data.fd == serverFd) {
            acceptNewClient();
        } else {
            messages.push_back(receiveFromClient(events[i].data.fd));
        }
    }
    return messages;
}

void NetworkLayer::setNonBlocking(int fd) {
    // TODO: Implement
}

void NetworkLayer::acceptNewClient() {
    // TODO: Implement
}

ClientMessage NetworkLayer::receiveFromClient(int fd) {
    // TODO: Implement
    return ClientMessage{};
}
