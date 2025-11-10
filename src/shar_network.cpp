#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "shar_network.h"

shar_network::shar_network(CONFIG config_, int threadNum):
port(config_.audioport)
{
    workPool = new Workpool(config_, threadNum);
    wsockFd = 0;
    epollfd = 0;
}

shar_network::~shar_network()
{
    if(workPool){
        delete workPool;
        workPool = nullptr;
    }

    close(wsockFd);
    close(epollfd);
}

bool shar_network::init_bindAdd_and_listen()
{
    wsockFd = socket(AF_INET, SOCK_STREAM, 0);
	if(wsockFd < 0){
		printf("CREATE SOCKET ERROR!\n");
		return false;
	}
    SetAddrReuse(wsockFd);

    serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port=htons(port);
	if(bind(wsockFd ,(struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
		perror("bind");
        close(wsockFd);
		return false;
	}

	listen(wsockFd, LISTEN_MAXI);
	return true;
}

int shar_network::setnonblocking(const int gSocketFd)
{
	int old_option =fcntl(gSocketFd,F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(gSocketFd,F_SETFL, new_option);
	return old_option;
}

bool shar_network::SetAddrReuse(const int gSocketFd)
{//避免出现“Address already in use”错误
	int val =1;
	setsockopt(gSocketFd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&val,sizeof(val));//允许服务器端口在关闭后快速重启，不必等 TIME_WAIT 结束

	if (0 == setsockopt(gSocketFd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val))){//允许多个 socket（可以属于不同进程或线程）同时绑定同一个端口
		printf("SET ADDR REUSE SUCCESS!\n");
		return true;
	}else {
        printf("SET ADDR REUSE FAILE!\n");
		return false;
	}
}


bool shar_network::start_sharNetwork()
{
	if(!init_bindAdd_and_listen()){
        return false;
    }

    if((epollfd = epoll_create(LISTEN_MAXI)) < 0){
		perror("WEBSOCKET EPOLL Create ");
		close(wsockFd);
		return false;
	}

    addfd(wsockFd, false);
    run();

    close(wsockFd);
    close(epollfd);
    return true;
}

bool shar_network::addfd(int fd, bool oneshot)
{
    struct epoll_event event = {0};
    memset(&event, 0, sizeof(epoll_event));
	event.data.fd = fd;
	event.events = EPOLLIN;
	if(oneshot){
		event.events |= EPOLLONESHOT;
	} else {
        setnonblocking(fd);
        event.events |= EPOLLET;
    }
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

    return true;
}

void shar_network::set_fd_keepalive(int fd)
{
    int keepAlive = 1; 
    int keepIdle = 60; 
    int keepInterval = 5; 
    int keepCount = 3; 

    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

    unsigned int timeout = 1000;//设置TCP_USER_TIMEOUT参数来判断tcp连接是否断开
    setsockopt(fd, IPPROTO_TCP, TCP_USER_TIMEOUT, &timeout, sizeof(timeout));
}

void shar_network::run()
{
    int reWait = 0;
    int index  = 0;
    int fd     = 0;
    int _fd    = 0;
    epoll_event events[EVENTS_MAXIM];
    sockaddr_in clientaddr;
    socklen_t m_clilen;
    m_clilen = sizeof(clientaddr);

    for(;;){
        reWait = epoll_wait(epollfd , events, EVENTS_MAXIM , -1);
        for(index =0; index < reWait; index++){
            fd = events[index].data.fd;

            if(fd == wsockFd){
                _fd = accept(fd, (struct sockaddr *)&clientaddr, &m_clilen);
                if(_fd < 0){
                    printf("accept connect fail (_fd<0).\n");
                    continue;
                }
#if DEBUG
                char ip[32] = {0};
                inet_ntop(AF_INET, &clientaddr.sin_addr, ip, sizeof(ip));
                printf("accept fd=%d, from %s:%d\n", _fd, ip, ntohs(clientaddr.sin_port));
#endif
                set_fd_keepalive(_fd);
                addfd(_fd,true);
            }else if(events[index].events & EPOLLIN){
                workPool->add_to_queue(fd);
                epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
            }else if(events[index].events & (EPOLLIN | EPOLLRDHUP)){
                printf("close fd.\n");
            }
        }
    }
}
