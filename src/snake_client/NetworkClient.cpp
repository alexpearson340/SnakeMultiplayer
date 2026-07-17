#include "snake_client/NetworkClient.h"
#include "common/Log.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

NetworkClient::NetworkClient(const std::string & host, int port) : serverFd {-1}, messageBuffer {} {
    connectToServer(host, port);
}

NetworkClient::~NetworkClient() {
    if (serverFd != -1) {
        close(serverFd);
    }
}

void NetworkClient::connectToServer(const std::string & host, int port) {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        throw std::runtime_error("Failed to create socket");
    }
    spdlog::info("New client serverFd=" + std::to_string(serverFd));

    // allow address reuse
    int opt {1};
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        close(serverFd);
        throw std::runtime_error("Invalid address: " + host);
    }

    if (connect(serverFd, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
        close(serverFd);
        throw std::runtime_error("Connection failed to " + host + ":" + std::to_string(port));
    }

    setNonBlocking(serverFd);
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

void NetworkClient::sendToServer(const Bytes & msgString) {
    size_t size {msgString.size()};

    ssize_t sent = send(serverFd, msgString.data(), size, 0);
    if (0 <= sent && static_cast<size_t>(sent) < size) {
        throw std::runtime_error(fmt::format("Partial send to server, tried to send {} bytes, actually sent {}, exiting", size, sent));
    }
    else if (sent == -1) {
        throw std::runtime_error(fmt::format("Error receieved {} on send to server, exiting", errno));
    }
}

std::vector<Bytes> NetworkClient::receiveFromServer() {
    char buffer[4096];
    ssize_t bytesRead = recv(serverFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return {};
        }
        throw std::runtime_error(fmt::format("Error on recv from server, errno {}", errno));
    }

    if (bytesRead == 0) {
        throw std::runtime_error(fmt::format("Server disconnected, exiting"));
    }

    return parseReceivedPacket(buffer, static_cast<size_t>(bytesRead));
}

std::vector<Bytes> NetworkClient::parseReceivedPacket(char * buffer, size_t size) {
    std::vector<Bytes> messages {};
    messageBuffer += std::string(buffer, size);
    size_t pos;
    while ((pos = messageBuffer.find('\n')) != std::string::npos) {
        messages.emplace_back(messageBuffer.substr(0, pos));
        messageBuffer.erase(0, pos + 1);
    }
    return messages;
}

void NetworkClient::waitForReadable(const int timeoutMs) {
    pollfd pfd {serverFd, POLLIN, 0};
    poll(&pfd, 1, timeoutMs);
}
