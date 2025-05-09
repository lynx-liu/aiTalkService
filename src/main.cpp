#include "rtpServer.h"
#include <signal.h>

CONFIG ServerConfig;
int main(int argc, char* argv[])
{
    get_config(&ServerConfig);

    signal(SIGPIPE, SIG_IGN);

    shared_ptr<RtpServer> _RtpServer = make_shared<RtpServer>(ServerConfig);
    _RtpServer->StartRTPServer_new();

    return 0;
}
