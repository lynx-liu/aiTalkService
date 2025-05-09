#ifndef _RTP_SERVER_H_
#define _RTP_SERVER_H_
#include "StreDataType.h"
#include "RTPServerEngine.h"
#include "videoWorkpool.h"
#include "shar_network.h"

#define EPOLL_MONITOR_MAXI 1024
#define EPOLL_EVENTS_MAIX  64
#define LISTEN_MONITOR_MAXI 1024
#define EPOLL_WAIT_TIMEOUT 100

#define SOCKETFD_TAG_START 1
#define SOCKETFD_TAG_WEBSERVER 2
#define SOCKETFD_TAG_SERVER1078 3
#define SOCKETFD_TAG_SERVERHTTP 4


struct EpollEvents_N;
class RtpServer{
public:
    RtpServer(const CONFIG ServerConfig);
    virtual ~RtpServer();
    int StartRTPServer();
    int StartRTPServer_new();

private:
    CONFIG Config;
    bool BindServerAdd(/*const int gSocketFd*/);
    bool SetAddrReuse(const int gSocketFd);
    void delete_ctl_event(int fd);

    bool CreateConne();
    bool addfd(int fd, bool oneshot, int type_);
    bool addfd2(int fd, bool oneshot);
    int setnonblocking_new(const int gSocketFd);
    int set_fd_block(const int _fd);
    void reset_oneshot(int fd, epollevent* eventptr_);
    void start_init();

private:
    int    m_gIndex;
    string m_ServerIp;
    int    m_ServerPort;
    string UrlKey;
    string urlDNS;
    int    cout;

    int    m_socketFd;
    int    m_fcntlFd;
    int    m_epollFd;
    int    m_ReadSocketFd;
    socklen_t m_clilen;
    struct epoll_event ev;
    // struct epoll_event events[EPOLL_EVENTS_MAIX];
    struct epoll_event* events;
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    epoll_event* event;

    //webServer
private:
    int            m_WebSerPort;
    int            websockFd;
    // Cwebserver*    webSerPtr;
    videoWorkpool* _videoWorkpool;
    epollevent*    eventptr;
    // epollevent*    _eventptr;
    // eventmempool*  eventmempoolptr;
    // Mutex          event_mutex2;
    // Cond           cond2;
    // std::list<epollevent*> event_List2;

    shar_network* sharNetwork;
};

#endif
