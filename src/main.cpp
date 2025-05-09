#include <signal.h>
#include "shar_network.h"

CONFIG ServerConfig;
int main(int argc, char* argv[])
{
    get_config(&ServerConfig);

    signal(SIGPIPE, SIG_IGN);

    shared_ptr<shar_network>  sharNetwork = make_shared<shar_network>(ServerConfig, 20);
    sharNetwork->start_sharNetwork();

    return 0;
}
