#include <signal.h>
#include <memory>
#include <thread>
#include "tiny_ws.h"
#include "shar_network.h"

CONFIG ServerConfig;
int main(int argc, char* argv[])
{
    get_config(&ServerConfig);

    signal(SIGPIPE, SIG_IGN);

    std::thread([=](){tiny_ws::start(ServerConfig.wsport);}).detach();

    shared_ptr<shar_network>  sharNetwork = make_shared<shar_network>(ServerConfig, 20);
    sharNetwork->start_sharNetwork();

    return 0;
}
