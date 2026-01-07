#pragma once

#include <string>

class NetworkClient {
public:
    NetworkClient(const std::string& host, int port);
    ~NetworkClient();

    void send(std::string_view msg);
    std::string receive();
    bool isConnected() const;

private:
    void connectToServer(const std::string& host, int port);
    void setNonBlocking(int fd);

    int serverFd;
    bool connected;
};
