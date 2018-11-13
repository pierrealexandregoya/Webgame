



#include "log.hpp"

#include "server.hpp"

#include "behavior.hpp"





int main(int ac, char **av)
{
    std::cout << std::boolalpha;

    unsigned short  port = 2000;
    unsigned int    threads = std::thread::hardware_concurrency();

    try {
        if (ac >= 2)
            port = std::stoi(av[1]);
        if (ac >= 3)
            threads = std::stoi(av[2]);

        std::make_shared<server>(port, threads)->run();
    }
    catch (std::exception const& e) {
        _MY_LOG("Std exception: " << e.what());
    }
    catch (boost::exception const&) {
        _MY_LOG("Boost exception");
    }
    catch (...) {
        _MY_LOG("Other exception");
    }
#ifdef _WIN32
    system("PAUSE");
#endif
    return 0;
}
