#include <sys/epoll.h>

class NetworkLayer {
public:
    NetworkLayer();
    void pollMessages();
};
