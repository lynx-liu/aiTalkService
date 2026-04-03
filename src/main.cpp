#include <signal.h>
#include <memory>
#include <thread>
#include "tiny_ws.h"
#include "shar_network.h"

CONFIG ServerConfig;
int main(int argc, char* argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);//设置printf无缓冲,否则默认要等到换行或缓冲区满才输出
    setvbuf(stderr, NULL, _IONBF, 0);

    get_config(&ServerConfig);

    signal(SIGPIPE, SIG_IGN);

    std::thread([=](){tiny_ws::start(ServerConfig.wsport);}).detach();

    shared_ptr<shar_network>  sharNetwork = make_shared<shar_network>(ServerConfig);
    sharNetwork->start_sharNetwork();

    return 0;
}
