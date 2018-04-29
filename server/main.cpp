#include "Server.hpp"

int main(int ac, char **av)
{
    unsigned short port = 2000;
    if (ac >= 2)
	port = std::atoi(av[1]);
    Server server(port);
    server.run();

    return 0;
}
