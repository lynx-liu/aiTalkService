#ifndef _SHAR_NETWORK_H
#define _SHAR_NETWORK_H
#include <netinet/tcp.h>
#include "StreDataType.h"
#include "shar_videoWorkpool.h"

#define LISTEN_MAXI 1024
#define EVENTS_MAXIM 32

class shar_network
{
public:
    shar_network(int gport, CONFIG config_, int threadNum);
    ~shar_network();
    bool start_sharNetwork();

private:
    bool init_bindAdd_and_listen();
    int setnonblocking(const int gSocketFd);
    bool SetAddrReuse(const int gSocketFd);
    bool addfd(int fd, bool oneshot);
    void set_fd_keepalive(int fd);
    bool create_listen();
    static void* _listen(void* arg);
    void run();
private:
    int port;
    struct sockaddr_in serveraddr;
    epoll_event* event;
    int wsockFd;
    int epollfd;

    videoWorkpool_S* workPool_S;
};


#endif