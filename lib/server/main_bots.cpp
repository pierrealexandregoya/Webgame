#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <list>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <webgame/log.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;

#define CHECK_DO(ws, e, msg, action)\
if (e)\
{\
    WEBGAME_LOG(std::to_string(ws->next_layer().local_endpoint().port()), e.message());\
    action;\
}\
else\
{\
    WEBGAME_LOG(std::to_string(ws->next_layer().local_endpoint().port()), msg);\
}

#define CHECK(ws, e, message) CHECK_DO(ws, e, message, )

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
        std::cerr << "Usage: " << av[0] << " <host> <port> <number of threads> <number of players per thread>" << std::endl;
        return 1;
    }
    nb_players_per_thread = std::atol(av[4]);
    nb_threads = std::atol(av[3]);
    port = av[2];
    host = av[1];

    webgame::title_max_size = 5;

    try
    {
        asio::io_context ioc;

        // RESOLVE
        asio::ip::tcp::resolver resolver(ioc);
        beast::error_code ec1;
        auto const results = resolver.resolve(host, port, ec1);
        if (ec1)
        {
            std::cerr << "Could not resolve " << host << ":" << port << ": " << ec1.message() << std::endl;
            return 1;
        }

        // THREADS
        std::vector<std::thread> threads;
        threads.reserve(nb_threads);
        for (auto i = 0; i < nb_threads; ++i)
            threads.emplace_back([&results] {

            asio::io_context ioc;

            try {
                // WS SOCKETS
                using socket_type = beast::websocket::stream<asio::ip::tcp::socket>;
                std::vector<std::shared_ptr<socket_type>> sockets;
                // sockets.reserve(nb_players_per_thread);
                beast::error_code ec;
                for (auto j = 0; j < nb_players_per_thread; ++j)
                {
                    // NEW SOCKET
                    auto ws = std::make_shared<socket_type>(ioc);

                    // CONNECT
                    asio::connect(ws->next_layer(), results.begin(), results.end(), ec);
                    CHECK_DO(ws, ec, "CONNECTED", continue);

                    // HANDSHAKE
                    ws->handshake(host, "/", ec);
                    CHECK_DO(ws, ec, "HANDSHAKED", ws->close(beast::websocket::close_code::abnormal); continue);

                    // AUTHENTICATION
                    std::string const playername = gen_name();
                    ws->write(asio::buffer("{\"order\":\"authentication\", \"player_name\":\"" + playername + "\"}"), ec);
                    CHECK_DO(ws, ec, "AUTHENTICATED: " << playername, ws->close(beast::websocket::close_code::abnormal); continue);

                    // READING STATE
                    beast::multi_buffer buffer;
                    ws->read(buffer, ec);
                    CHECK_DO(ws, ec, "FIRST READ", ws->close(beast::websocket::close_code::abnormal, ec); continue);

                    // SETTING SPEED
                    ws->write(asio::buffer("{\"order\":\"action\", \"suborder\":\"change_speed\", \"speed\":0.1}"), ec);
                    CHECK_DO(ws, ec, "SPEED CHANGED", ws->close(beast::websocket::close_code::abnormal); continue);

                    sockets.push_back(std::make_shared<socket_type>(std::move(ws_tmp)));
                }

                // LOOP
                auto now = std::chrono::steady_clock::now();
                auto write_time = 0.;
                auto close_time = 0.;
                for (;;)
                {
                    // TIME
                    auto new_now = std::chrono::steady_clock::now();
                    double d = std::chrono::duration_cast<std::chrono::duration<double>>(new_now - now).count();
                    now = new_now;

                    // STOP AND CLOSE ALL SOCKETS OF THIS THREAD AFTER 20 SECS
                    close_time += d;
                    if (close_time >= 40.f)
                        break;

                    write_time += d;

                    auto it = std::remove_if(sockets.begin(), sockets.end(), [](std::shared_ptr<socket_type> const& ws) {
                        return !ws->is_open();
                    });
                    sockets.erase(it, sockets.end());
                    if (sockets.empty())
                        break;

                    // WRITE RANDOM DIRECTION EVERY 2 SECS
                    if (write_time >= 0.25f)
                    {
                        for (auto &ws : sockets)
                        {
                            std::string text = "{\"order\": \"action\", \"suborder\": \"change_dir\", \"dir\": {\"x\": " + std::to_string(dis(g)) + ", \"y\": " + std::to_string(dis(g)) + "}}";
                            WEBGAME_LOCK(webgame::log_mutex);
                            CHECK(ws, ec, "WRITING");
                            ws->write(asio::buffer(text), ec);
                            CHECK_DO(ws, ec, "DIRECTION CHANGED", ws->close(beast::websocket::close_code::abnormal, ec); continue);
                        }
                        write_time = 0.f;
                    }

                    // READ ALL
                    for (auto &ws : sockets)
                    {
                        beast::multi_buffer buffer;
                        WEBGAME_LOCK(webgame::log_mutex);
                        CHECK(ws, ec, "READING");
                        ws->read(buffer, ec);
                        CHECK_DO(ws, ec, "READ " << boost::beast::buffers_to_string(buffer.data()), ws->close(beast::websocket::close_code::abnormal, ec); continue);
                    }
                }

                // CLOSE
                for (auto &ws : sockets)
                {
                    WEBGAME_LOCK(webgame::log_mutex);
                    CHECK(ws, ec, "CLOSING");
                    ws->close(beast::websocket::close_code::normal, ec);
                    WEBGAME_LOG("", "CLOSED");
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
