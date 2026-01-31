#pragma once

#include <string>
#include "common/Constants.h"
#include "common/ProtocolMessage.h"

inline const char * getServerIp() {
    const char * ip = getenv("SNAKE_SERVER_IP");
    return ip ? ip : "127.0.0.1";
}

inline int getServerPort() {
    const char * port = getenv("SNAKE_SERVER_PORT");
    return port ? std::atoi(port) : SERVER_PORT;
}

class NetworkClient {
public:
    NetworkClient(const std::string& host, int port);
    ~NetworkClient();

    int getServerFd() const {return serverFd;};
    void sendToServer(std::string_view msg);
    std::vector<ProtocolMessage> receiveFromServer();

private:
    void connectToServer(const std::string& host, int port);
    void setNonBlocking(int fd);
    std::vector<ProtocolMessage> parseReceivedPacket(char* buffer, size_t size);

    int serverFd;
    bool connected;
    std::string messageBuffer;
};
