#include <thread>

#include <boost/asio/io_context.hpp>

#include <webgame/behavior.hpp>
#include <webgame/lock.hpp>
#include <webgame/log.hpp>
#include <webgame/player.hpp>
#include <webgame/redis_persistence.hpp>
#include <webgame/server.hpp>

void show_help()
{
    _WEBGAME_MY_LOG("commands:" << std::endl
        << "\thelp" << std::endl
        << "\tinfo" << std::endl
        << "\texit" << std::endl
        << "\tio: set/unset io log: " << webgame::io_log << std::endl
        << "\tdata: set/unset data log (no effect if io log is unset): " << webgame::data_log
    );
}

void handle_user_input(std::shared_ptr<webgame::server> server_p)
{
    std::string input;
    for (;;)
    {
        std::cin >> input;
        if (input == "info")
        {
            WEBGAME_LOCK(webgame::log_mutex);
            WEBGAME_LOG("INFO", "Connections: " << server_p->get_connections().size());
            WEBGAME_LOG("INFO", "Entities: " << server_p->get_entities().size());
            /*LOG("INFO", "Threads: " << network_threads_.size() << " asio thread"
            << (network_threads_.size() > 1 ? "s" : "") << " + user input thread + game loop thread");*/
        }
        else if (input == "help")
        {
            WEBGAME_LOCK(webgame::log_mutex);
            _WEBGAME_MY_LOG("");
            show_help();
            _WEBGAME_MY_LOG("");
        }
        else if (input == "data")
            webgame::data_log = !webgame::data_log;
        else if (input == "io")
            webgame::io_log = !webgame::io_log;
        else if (input == "exit")
        {
            server_p->shutdown();
            return;
        }
        else
        {
            WEBGAME_LOCK(webgame::log_mutex);
            _WEBGAME_MY_LOG("");
            _WEBGAME_MY_LOG("Unknown command: " << input);
            _WEBGAME_MY_LOG("");
            show_help();
            _WEBGAME_MY_LOG("");
        }
    }
}

int main(int ac, char **av)
{
    std::cout << std::boolalpha;

    unsigned short  port = 2000;
    unsigned int    nb_threads = std::thread::hardware_concurrency();

    if (ac >= 2)
        port = std::stoi(av[1]);
    if (ac >= 3)
        nb_threads = std::stoi(av[2]);


    boost::asio::io_context ioc;

    std::vector<std::thread> network_threads;

    try {
        auto game_server = std::make_shared<webgame::server>(ioc, port, std::make_shared<webgame::redis_persistence>(ioc, "localhost", 6379, 10), "test-server", [](webgame::entities &ents) {
            return std::make_shared<webgame::upview_player>();
        });

        game_server->start();

        for (unsigned int i = 0; i < nb_threads; ++i)
            network_threads.emplace_back([&ioc, i] {
            WEBGAME_LOG("STARTUP", "THREAD #" << i + 1 << " RUNNING");
            try {
                ioc.run();
            }
            catch (std::exception const& e) {
                _WEBGAME_MY_LOG("EXCEPTION THROWN: " << e.what());
            }
            WEBGAME_LOG("STARTUP", "THREAD #" << i + 1 << " STOPPED");
        });

        handle_user_input(game_server);
    }
    catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
    }

    for (std::thread & t : network_threads)
        t.join();

#ifdef _WIN32
    system("PAUSE");
#endif

    return 0;
}
