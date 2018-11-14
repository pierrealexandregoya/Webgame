#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "any.hpp"
#include "nmoc.hpp"
#include "persistence.hpp"

namespace webgame {

class player;
class server;

class player_conn : public std::enable_shared_from_this<player_conn>
{
public:
    struct patch
    {
        std::string what;
        any         value;

        patch(std::string &&w, any &&v);
        patch(patch &&other) = default;
        patch & operator=(patch &&) = default;

        patch(patch const&) = delete;
        patch & operator=(patch const&) = delete;
    };

    enum state
    {
        none,
        ready,
        handshaking,
        authenticating,
        loading_player,
        reading,
        writing,
        to_be_closed,
        closing,
        closed
    };

public:
    static std::vector<std::string> const               state_str;

    std::string const                                   addr_str;

private:
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket>     socket_;
    boost::asio::strand<boost::asio::io_context::executor_type> const strand_;
    state                                                             state_;
    boost::beast::multi_buffer                                        read_buffer_;
    std::list<std::shared_ptr<std::string const>>                     to_write_;
    std::shared_ptr<player>                                           player_entity_;
#ifndef WEBGAME_MONOTHREAD
    std::recursive_mutex                                              handlers_mutex_;
#endif /* !WEBGAME_MONOTHREAD */
    std::queue<patch>                                                 patches_;
#ifndef WEBGAME_MONOTHREAD
    mutable std::recursive_mutex                                      patches_mutex_;
#endif /* !WEBGAME_MONOTHREAD */
    boost::beast::websocket::close_code                               close_code_;
    std::string                                                       player_name_;
    std::shared_ptr<server>                                           server_;
    boost::asio::steady_timer                                         close_timer_;

private:
    WEBGAME_NON_MOVABLE_OR_COPYABLE(player_conn);

public:
    player_conn(boost::asio::ip::tcp::socket &&socket, std::shared_ptr<server> const& server);

    void                            start();
    void                            write(std::shared_ptr<std::string const> msg);
    void                            close();
    patch                           pop_patch();
    bool                            has_patch() const;

    bool                            is_closed() const;
    std::shared_ptr<player> const&  player_entity() const;
    state                           current_state() const;
    bool                            is_ready() const;
    std::string const&              player_name() const;

private:
    void write_next();

    void on_accept(boost::system::error_code const& ec) noexcept;
    void on_read(boost::system::error_code const& error, std::size_t const& bytes_transferred) noexcept;
    void on_write(boost::system::error_code const& ec, std::size_t const& bytes_transferred) noexcept;
    void on_close() noexcept;
    void on_player_load(std::shared_ptr<player> const& player_entity);

    void do_read();
    void do_close(boost::beast::websocket::close_code const& code);

    void interpret(std::string &&order_str);

    void push_patch(std::string && what, any && value);
    void push_patch(patch && p);
};

} // namespace webgame
