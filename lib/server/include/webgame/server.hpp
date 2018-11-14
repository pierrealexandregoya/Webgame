#pragma once

#include <future>
#include <mutex>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include "common.hpp"
#include "config.hpp"
#include "containers.hpp"
#include "entities.hpp"
#include "nmoc.hpp"
#include "time.hpp"

namespace webgame {

class persistence;
class player;
class vector;

class WEBGAME_API server : public std::enable_shared_from_this<server>
{
private:
    typedef std::chrono::duration<double, std::ratio<1>> delta_duration;

private:
    connections                              conns_;
    entities                                 entities_;
    boost::asio::io_context&                 io_context_;
    boost::asio::ip::tcp::endpoint           local_endpoint_;
    boost::asio::ip::tcp::acceptor           acceptor_;
    boost::asio::ip::tcp::socket             new_client_socket_;
    std::shared_ptr<persistence>             persistence_;
    steady_clock::duration                   tick_duration_;
    steady_clock::time_point                 wake_time_;
#ifndef NDEBUG
    steady_clock::time_point                 start_time_;
#endif /* !NDEBUG */
#ifndef WEBGAME_MONOTHREAD
    std::recursive_mutex                     server_mutex_;
#endif /* !WEBGAME_MONOTHREAD */
    std::shared_ptr<bool>                    stop_;
    boost::asio::steady_timer                game_cycle_timer_;

    WEBGAME_NON_MOVABLE_OR_COPYABLE(server);

public:
    server(boost::asio::io_context &io_context, unsigned int port, std::shared_ptr<persistence> const& persistence);

    template<class Rep = int, class Per = std::milli>
    void start(std::chrono::duration<Rep, Per> const& tick_duration = std::chrono::duration<Rep, Per>(250))
    {
        *stop_ = false;

        tick_duration_ = std::chrono::duration_cast<decltype(tick_duration_)>(tick_duration);

        start_persistence();

        start_game();

        start_network();
    }

    void                            shutdown();
    bool                            is_player_connected(std::string const& name);
    void                            register_player(std::shared_ptr<player_conn> const& conn, std::shared_ptr<player> const& new_ent);
    std::shared_ptr<persistence>    get_persistence();
    connections const&              get_connections() const;
    entities const&                 get_entities() const;

private:
    void    start_persistence();
    void    start_game();
    void    start_network();

    void    game_cycle(boost::system::error_code const& error, unsigned int nb_ticks);

    void    on_accept(const boost::system::error_code& error) noexcept;
    void    do_accept();
};

} // namespace webgame
