#include "rtpServer.h"

RtpServer::RtpServer(const CONFIG ServerConfig):
m_ServerIp(ServerConfig.serverip),
m_ServerPort(ServerConfig.prot),
m_WebSerPort(ServerConfig.wprot)
{
	Config = ServerConfig;

	//网络部分
    m_socketFd = 0;
    m_fcntlFd  = -1;
    m_epollFd  = -1;
    m_ReadSocketFd = -1;
    m_clilen = sizeof(clientaddr);
    memset(&ev, 0, sizeof(ev));
    memset(&serveraddr, 0, sizeof(serveraddr));
    memset(&clientaddr, 0, sizeof(clientaddr));
    events = new epoll_event[EPOLL_EVENTS_MAIX];
	event = new epoll_event();

	_videoWorkpool = new videoWorkpool(ServerConfig, 10, 20);

	sharNetwork = new shar_network(m_WebSerPort, ServerConfig, 20);

}

void RtpServer::start_init()
{
	
}

RtpServer::~RtpServer()
{
	if(nullptr != events){
		delete [] events;
		events = nullptr;
	}
	if(event){
		delete event;
		event = nullptr;
	}

	if(nullptr != _videoWorkpool){
		delete _videoWorkpool;
		_videoWorkpool = nullptr;
	}

	if(sharNetwork){
		delete sharNetwork;
		sharNetwork = nullptr;
	}

	close(m_socketFd); 
    close(m_epollFd);
}


bool RtpServer::BindServerAdd(/*const int gSocketFd*/)
{
	m_socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if(m_socketFd < 0){
		// printf("CREATE SOCKET FAILE!.exit\n");
		exit(1);
	}
	SetAddrReuse(m_socketFd);

	serveraddr.sin_family = AF_INET;
	// inet_aton(m_ServerIp.c_str(),&(serveraddr.sin_addr));
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port=htons(m_ServerPort);
	if(bind(m_socketFd ,(struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
		perror("BIND SERVER ADDR FAIL! ");
		// LOG(INFO) << "BIND SERVER ADDR FAIL! ";
		exit(1);
		return false;
	}
	// else LOG(INFO) <<"BIND SERVER ADDR SUCCESS!";
	listen(m_socketFd, LISTEN_MONITOR_MAXI);
	return true;
}

bool RtpServer::SetAddrReuse(const int gSocketFd)
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
		switch(errno)
		{
			case EBADF:
				// LOG(INFO) <<"gSocketFd OF NO AVAIL";
				break;
			case EFAULT:
				// LOG(INFO) <<"bReuseaddr OF NO AVAIL SPACE";
				break;
			case EINVAL:
				// LOG(INFO) <<"optlen(sizeof(bReuseaddr)) OF NO AVAIL";
				break;
			case ENOPROTOOPT:
				// LOG(INFO) <<"protocol Unable to identify";
				break;
			case ENOTSOCK:
				// LOG(INFO) <<"Not A SOCKET";
				break;
			default:
				break;
		}
		return false;
	}
}


int RtpServer::StartRTPServer_new()
{
	int countt = 0;
	int fd_     = 0;
	int cfd_    = 0;
	int rewait = 0;
	int index   = 0;

	sharNetwork->start_sharNetwork();
	if(CreateConne()){
		for(;;){
			memset(events, 0, EPOLL_EVENTS_MAIX);
			rewait = epoll_wait(m_epollFd , events, EPOLL_EVENTS_MAIX , -1);//EPOLL_WAIT_TIMEOUT
			if(rewait < 0) {
				// LOG(INFO)<< "rewait == -1 ERROR";
				continue;
			}

			for(index =0; index < rewait; index++){
				fd_ = events[index].data.fd;
				if(fd_ == m_socketFd ){                                                  
					cfd_ = accept(fd_, (struct sockaddr *)&clientaddr, &m_clilen);
					if(cfd_ < 0){
						// LOG(INFO) << "accept CONNENT fd < 0 ERROR"; 
						continue;
					}
					addfd2(cfd_, true);
					

				}else if(events[index].events & EPOLLIN){       
					_videoWorkpool->add_device_detonate_event2(fd_);
					delete_ctl_event(fd_);

					// LOG(INFO)<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~接收连接countt = "<<++countt;
					// printf("---------CLOSE-------, fd_ = %d\n", fd_);

				}else if(events[index].events & (EPOLLIN | EPOLLRDHUP)){
					// printf("---------CLOSE-------\n");
				}
			}
		}
	}
	return 0;
}


bool RtpServer::addfd2(int fd, bool oneshot)
{
	setnonblocking_new(fd);

	// epoll_event event;
	memset(event, 0, sizeof(epoll_event));
	event->events = EPOLLIN;
	if(oneshot){
		event->events |= EPOLLET;
	}
	event->data.fd = fd;
	epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, event);
	return true;
}

void RtpServer::delete_ctl_event(int fd)
{
    struct epoll_event ev_del;
	epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, &ev_del);
}

int RtpServer::setnonblocking_new(const int gSocketFd)
{
	int old_option =fcntl(gSocketFd,F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(gSocketFd,F_SETFL, new_option);
	return old_option;
}
int RtpServer::set_fd_block(const int _fd)
{
	int _ret = fcntl(_fd, F_SETFL, 0);
	return _ret;
}

bool RtpServer::CreateConne()
{
	BindServerAdd(/*m_socketFd*/);

	if((m_epollFd = epoll_create(EPOLL_MONITOR_MAXI)) < 0){
		printf("EPOLL Create Faile, exit!\n");
		close(m_socketFd);
		exit(1);
	}else perror("EPOLL Create ");
	printf(" m_socketFd = %d, m_epollFd=%d\n",  m_socketFd, m_epollFd);
	// addfd(m_socketFd, false, SOCKETFD_TAG_START);
	addfd2(m_socketFd, false);
	
	return true;
}
