#include <string>
#include <unordered_map>
#include <vector>

inline int MAX_EVENTS {100}; 

struct ClientMessage {
    int clientId; 
    std::string data;
};

class NetworkLayer {
public:
    NetworkLayer (int);
    
    std::vector<ClientMessage> pollMessages ();
private: 
    void setNonBlocking(int fd);
    void registerFdWithEpoll(int fd);
    void acceptNewclient();
    ClientMessage receiveFromClient(int fd);
    
    int serverFd; 
    int epollFd;
    int nextClientid;
    std::unordered_map<int, int> fdToclientIdMap;
}