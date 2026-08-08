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

void NetworkClient::sendToServer(const Bytes & bytes) {
    uint32_t len {static_cast<uint32_t>(bytes.size())};
    Bytes frame {};
    frame.reserve(sizeof(len) + bytes.size());
    frame.append(reinterpret_cast<const char *>(&len), sizeof(len));
    frame += bytes;
    ssize_t sent = send(serverFd, frame.data(), frame.size(), 0);

    if (0 <= sent && static_cast<size_t>(sent) < frame.size()) {
        throw std::runtime_error(fmt::format(
            "Partial send to server, tried to send {} bytes, actually sent {}, exiting", frame.size(), sent));
    } else if (sent == -1) {
        throw std::runtime_error(fmt::format("Error receieved {} on send to server, exiting", errno));
    }
}

std::vector<Bytes> NetworkClient::receiveFromServer() {
    ssize_t bytesRead = recv(serverFd, recvBuffer, sizeof(recvBuffer), 0);

    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return {};
        }
        throw std::runtime_error(fmt::format("Error on recv from server, errno {}", errno));
    }

    if (bytesRead == 0) {
        throw std::runtime_error(fmt::format("Server disconnected, exiting"));
    }

    return parseReceivedPacket(recvBuffer, static_cast<size_t>(bytesRead));
}

std::vector<Bytes> NetworkClient::parseReceivedPacket(char * inputBuffer, size_t size) {
    std::vector<Bytes> frames {};
    messageBuffer += Bytes(inputBuffer, size);

    uint32_t len;
    while (messageBuffer.size() >= sizeof(len)) {
        memcpy(&len, messageBuffer.data(), sizeof(len));

        // no legit message should be bigger than this - it means we have desynced
        if (len > CLIENT_RECV_MAX_MESSAGE_SIZE) {
            throw std::runtime_error(
                fmt::format("Received message of size {}, which is bigger than maximum allowed {}. Aborting", len,
                            CLIENT_RECV_MAX_MESSAGE_SIZE));
        }

        // full frame not here yet - wait
        if (messageBuffer.size() < sizeof(len) + len) {
            break;
        }
        frames.push_back(messageBuffer.substr(sizeof(len), len));
        messageBuffer.erase(0, sizeof(len) + len);
    }
    return frames;
}

void NetworkClient::waitForReadable(const int timeoutMs) {
    pollfd pfd {serverFd, POLLIN, 0};
    poll(&pfd, 1, timeoutMs);
}
