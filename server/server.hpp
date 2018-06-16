#pragma once

#include <mutex>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "misc/common.hpp"
#include "misc/containers.hpp"
#include "misc/nmoc.hpp"
#include "misc/time.hpp"

class vector;

namespace asio = boost::asio;

class server : public std::enable_shared_from_this<server>
{
private:
    typedef std::chrono::duration<duration, std::ratio<1>> delta_duration;


private:
    connections                             conns_;
    entities                                entities_;
    unsigned int                            threads_;
    asio::io_context                        io_context_;
    asio::ip::tcp::acceptor                 acceptor_;
    asio::ip::tcp::socket                   new_client_socket_;
    steady_clock::duration                  const tick_duration_;
    steady_clock::time_point                wake_time_;
    std::recursive_mutex                    conns_mutex_;
    std::shared_ptr<bool>                   stop_;

    NON_MOVABLE_OR_COPYABLE(server);

public:
    server(unsigned int port, unsigned int threads = 1);

    void    run();
    void    loop(unsigned int nb_ticks);

private:
    void                    on_accept(const boost::system::error_code& error) noexcept;

    void                    do_accept();

    std::shared_ptr<entity> add_entity(vector const& pos, vector const& dir, real speed, real max_speed, std::string const& type, behaviors && behaviors = behaviors());
};
