#include "snake_client/NetworkClient.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

NetworkClient::NetworkClient(const std::string& host, int port)
    : serverFd{-1}
    , connected{false}
    , messageBuffer {} {
    connectToServer(host, port);
}

NetworkClient::~NetworkClient() {
    if (serverFd != -1) {
        close(serverFd);
    }
}

void NetworkClient::connectToServer(const std::string& host, int port) {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        close(serverFd);
        throw std::runtime_error("Invalid address: " + host);
    }

    if (connect(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverFd);
        throw std::runtime_error("Connection failed to " + host + ":" + std::to_string(port));
    }

    setNonBlocking(serverFd);
    connected = true;
}

void NetworkClient::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("fcntl F_GETFL failed");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("fcntl F_SETFL failed");
    }
}

void NetworkClient::sendToServer(std::string_view msg) {
    if (!connected) {
        throw std::runtime_error("Not connected to server");
    }

    ssize_t sent = send(serverFd, msg.data(), msg.size(), 0);
    if (sent < 0) {
        throw std::runtime_error("Send failed");
    }
}

std::vector<ProtocolMessage> NetworkClient::receiveFromServer() {
    if (!connected) {
        throw std::runtime_error("Not connected to server");
    }

    char buffer[4096];
    ssize_t bytesRead = recv(serverFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return std::vector<ProtocolMessage> {};
        }
        connected = false;
        throw std::runtime_error("Receive failed");
    }

    if (bytesRead == 0) {
        connected = false;
        throw std::runtime_error("Server disconnected");
    }

    return parseReceivedPacket(buffer, bytesRead);
}

std::vector<ProtocolMessage> NetworkClient::parseReceivedPacket(char* buffer, size_t size) {
    std::vector<ProtocolMessage> messages {};
    messageBuffer += std::string(buffer, size);
    size_t pos;
    while ((pos = messageBuffer.find('\n')) != std::string::npos) {
        std::string msg {messageBuffer.substr(0, pos)};
        ProtocolMessage pm {protocol::fromString(msg)};
        messages.push_back(pm);
        messageBuffer.erase(0, pos + 1);
    }
    return messages;
}

bool NetworkClient::isConnected() const {
    return connected;
}
