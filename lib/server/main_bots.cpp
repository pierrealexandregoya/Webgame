#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <webgame/log.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;

#define CHECK_DO(title, e, action)\
if (e)\
{\
    WEBGAME_LOG(title, e.message());\
    action;\
}\
else\
{\
    WEBGAME_LOG(title, "");\
}

#define CHECK(title, e) CHECK_DO(title, e, )

// We want deterministic random
std::mt19937 g(0);
std::uniform_real_distribution<> dis(-1.0, 1.0);


char const consonants[] = "bcdfghjklmnpqrstvwxz";
char const vowels[] = "aeiouy";

std::uniform_int_distribution<unsigned int> consonants_rd(0, sizeof(consonants) / sizeof(*consonants) - 2);
std::uniform_int_distribution<unsigned int> vowels_rd(0, sizeof(vowels) / sizeof(*vowels) - 2);

unsigned int const nb_syllabus = 5;

std::string gen_name()
{
    std::string name;
    name.reserve(nb_syllabus * 2 + 1);
    for (int i = 0; i < nb_syllabus; ++i)
    {
        name.push_back(consonants[consonants_rd(g)]);
        name.push_back(vowels[vowels_rd(g)]);
    }
    return name;
}

std::string host = "localhost";
std::string port = "2000";
std::size_t nb_threads = 1;
std::size_t nb_players_per_thread = 1;

int main(int ac, char **av)
{
    if (ac != 5)
    {
        std::cerr << "Usage: " << av[0] << "<host> <port> <number of threads> <number of players per thread>" << std::endl;
        return 1;
    }
    nb_players_per_thread = std::atol(av[4]);
    nb_threads = std::atol(av[3]);
    port = av[2];
    host = av[1];

    webgame::title_max_size = 30;

    try
    {
        auto const ioc = std::make_shared<asio::io_context>();

        // RESOLVE
        auto const resolver = std::make_shared<asio::ip::tcp::resolver>(*ioc);
        beast::error_code ec1;
        auto const results = std::make_shared<decltype(resolver)::element_type::results_type const>(resolver->resolve(host, port, ec1));
        CHECK_DO("RESOLVE", ec1, return 1);

        // THREADS
        std::vector<std::thread> threads;
        threads.reserve(nb_threads);
        for (auto i = 0; i < nb_threads; ++i)
            threads.emplace_back([&] {

            try {
                // WS SOCKETS
                using socket_type = beast::websocket::stream<asio::ip::tcp::socket>;
                std::vector<socket_type> sockets;
                sockets.reserve(nb_players_per_thread);
                beast::error_code ec;
                for (auto j = 0; j < nb_players_per_thread; ++j)
                {
                    // NEW SOCKET
                    auto ws = socket_type(*ioc);

                    // CONNECT
                    asio::connect(ws.next_layer(), results->begin(), results->end(), ec);
                    CHECK_DO("CONNECTION", ec, continue);

                    // HANDSHAKE
                    ws.handshake(host, "/", ec);
                    CHECK_DO("HANDSHAKE", ec, ws.close(beast::websocket::close_code::abnormal); continue);

                    // AUTHENTICATION
                    std::string const playername = gen_name();
                    ws.write(asio::buffer("{\"order\":\"authentication\", \"playername\":\"" + playername + "\"}"), ec);
                    CHECK_DO("AUTHENTICATION (" << playername << ")", ec, ws.close(beast::websocket::close_code::abnormal); continue);

                    sockets.push_back(std::move(ws));
                }

                // LOOP
                auto now = std::chrono::steady_clock::now();
                auto write_time = 0.;
                auto close_time = 0.;
                for (;;)
                {
                    // TIME
                    auto new_now = std::chrono::steady_clock::now();
                    double d = std::chrono::duration_cast<std::chrono::duration<double>>(new_now - now).count() / 1000000000.;
                    now = new_now;

                    // STOP AND CLOSE ALL SOCKETS OF THIS THREAD AFTER 20 SECS
                    close_time += d;
                    if (close_time >= 40.f)
                        break;

                    write_time += d;

                    auto it = std::remove_if(sockets.begin(), sockets.end(), [](socket_type const& ws) {
                        return !ws.is_open();
                    });
                    sockets.erase(it, sockets.end());
                    if (sockets.empty())
                        break;

                    // WRITE RANDOM DIRECTION EVERY 2 SECS
                    if (write_time >= 0.25f)
                    {
                        for (auto &ws : sockets)
                        {
                            std::string text = "{\"order\": \"state\", \"suborder\": \"player\", \"speed\":0.1, \"vel\": {\"x\": " + std::to_string(dis(g)) + ", \"y\": " + std::to_string(dis(g)) + "}}";
                            ws.write(asio::buffer(text), ec);
                            CHECK_DO("WRITE", ec, ws.close(beast::websocket::close_code::abnormal, ec); continue);
                        }
                        write_time = 0.f;
                    }

                    // READ ALL
                    for (auto &ws : sockets)
                    {
                        beast::multi_buffer buffer;
                        ws.read(buffer, ec);
                        CHECK_DO("READ", ec, ws.close(beast::websocket::close_code::abnormal, ec); continue);
                    }
                }

                // CLOSE
                for (auto &ws : sockets)
                {
                    ws.close(beast::websocket::close_code::normal, ec);
                    CHECK("CLOSE", ec);
                }
            }
            catch (...) {
                WEBGAME_LOG("EXCEPTION THROWN", "");
            }
        });

        // JOIN ALL THREADS
        for (auto &t : threads)
        {
            WEBGAME_LOG("MAIN THREAD", "Joining " << t.get_id());
            t.join();
        }
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
