#include "Server.hpp"

int main(int ac, char **av)
{
    //try {
        unsigned short port = 2000;
        if (ac >= 2)
            port = std::atoi(av[1]);
        Server server(port);
        server.run();
    //}
    //catch (std::exception const& e) {
    //    std::cout << "Std exception: " << e.what() << std::endl;
    //}
    //catch (boost::exception const&) {
    //    std::cout << "Boost exception" << std::endl;
    //}
    //catch (...) {
    //    std::cout << "Other exception" << std::endl;
    //}
    system("PAUSE");
    return 0;
}
