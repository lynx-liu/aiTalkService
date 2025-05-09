#include "shar_network.h"

shar_network::shar_network(int gport, CONFIG config_, int threadNum):
port(gport)
{
    workPool_S = nullptr;
    workPool_S = new videoWorkpool_S(config_, threadNum);
    event = new epoll_event();
    wsockFd = 0;
    epollfd = 0;
}

shar_network::~shar_network()
{
    if(workPool_S){
        delete workPool_S;
        workPool_S = nullptr;
    }

    if(event){
        delete event;
        event = nullptr;
    }

    close(wsockFd);
    close(epollfd);
}

bool shar_network::init_bindAdd_and_listen()
{
    wsockFd = socket(AF_INET, SOCK_STREAM, 0);
	if(wsockFd < 0){
		// LOG(INFO)<< "CREATE SOCKET ERROR!";
		return false;
	}
    // setnonblocking(wsockFd);
    SetAddrReuse(wsockFd);

    serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port=htons(port);
	if(bind(wsockFd ,(struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
		perror("BIND SERVER ADDR FAIL! ");
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
{
	int RetSetVal;
	int bReuseaddr=SO_REUSEADDR;
	RetSetVal = setsockopt(gSocketFd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(bReuseaddr));

    int val =1;
	RetSetVal = setsockopt(gSocketFd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
	if (0 == RetSetVal){
		// LOG(INFO) << "SET ADDR REUSE SUCCESS!";
		return true;
	}else {
        // LOG(INFO) << "SET ADDR REUSE FAILE!";
		return false;
	}
}


bool shar_network::start_sharNetwork()
{
	if(!init_bindAdd_and_listen()){
        return false;
    }

    if((epollfd = epoll_create(1024)) < 0){
		// LOG(INFO) << "WEBSOCKET EPOLL Create Fail.";
		close(wsockFd);
		exit(1);
	}else perror("WEBSOCKET EPOLL Create ");

    addfd(wsockFd, false);
    if(!create_listen()){
        // LOG(ERROR)<< "create listen pthread fail.";
        close(wsockFd);
        close(epollfd);
        return false;
    }
    return true;
}

bool shar_network::addfd(int fd, bool oneshot)
{
	setnonblocking(fd);

    // epoll_event event;
    memset(event, 0, sizeof(epoll_event));
	event->data.fd = fd;
	event->events = EPOLLIN;
	if(oneshot){
		event->events |= EPOLLET;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, event);

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

    unsigned int timeout = 1000;
    setsockopt(fd, IPPROTO_TCP, 18, &timeout, sizeof(timeout));
    //setsockopt(socket_fd, IPPROTO_TCP, TCP_USER_TIMEOUT, &timeout, sizeof(timeout));
	//设置TCP_USER_TIMEOUT参数来判断tcp连接是否断开
}

bool shar_network::create_listen()
{
    pthread_t listen_pid;
    if(pthread_create(&listen_pid,NULL,_listen,this)!=0){
        // LOG(ERROR)<< "websocke server create work thread fail";
        return false;
    }
    return true;
}

void* shar_network::_listen(void* arg)
{
    pthread_detach(pthread_self());
    shar_network* _this = (shar_network*)arg;

    _this->run();
    pthread_exit(NULL);
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
                    // LOG(ERROR)<<"accept connect fail (_fd<0).";
                    continue;
                }
                set_fd_keepalive(_fd);
                addfd(_fd,true);
                // LOG(INFO)<< "============= _fd1 = "<< _fd;
                // printf("============= _fd1 =%d\n", _fd);
            }else if(events[index].events & EPOLLIN){
                // LOG(INFO)<< "============= _fd2 = "<< fd;
                // printf("============= _fd2 =%d\n", fd);

                // webpoolObj->append_event(fd);
                workPool_S->add_device_detonate_event2(fd);
                epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
            }else if(events[index].events & (EPOLLIN | EPOLLRDHUP)){
                // LOG(INFO) << "close fd.";
            }
        }
    }
}
