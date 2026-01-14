#pragma once

#include <string>
#include "common/ProtocolMessage.h"

class NetworkClient {
public:
    NetworkClient(const std::string& host, int port);
    ~NetworkClient();

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
