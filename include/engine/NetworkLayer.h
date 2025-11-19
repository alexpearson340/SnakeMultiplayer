#include <string>
#include <vector>

inline int MAX_EVENTS {10};
struct ClientMessage {
    int clientId;
    std::string data;
};

class NetworkLayer {
public:
    NetworkLayer(int);
    std::vector<ClientMessage> pollMessages();

private:
    void setNonBlocking(int fd);
    void registerFdWithEpoll(int fd);
    void acceptNewClient();
    ClientMessage receiveFromClient(int fd);

    int serverFd;
    int epollFd;
    int nextClientId;
};
