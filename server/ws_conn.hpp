#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/beast/core/multi_buffer.hpp>

#include "misc/any.hpp"
#include "misc/nmoc.hpp"

namespace beast = boost::beast;
namespace asio = boost::asio;

class entity;

class ws_conn : public std::enable_shared_from_this<ws_conn>
{
public:
    typedef beast::websocket::stream<asio::ip::tcp::socket> socket_t;

    struct patch
    {
        std::string what;
        any         value;
    };

    enum state
    {
        none,
        ready,
        handshaking,
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
    socket_t                                            socket_;
    asio::strand<asio::io_context::executor_type> const strand_;
    state                                               state_;
    beast::multi_buffer                                 read_buffer_;
    std::list<std::shared_ptr<std::string const>>       to_write_;
    std::shared_ptr<entity const> const                 player_entity_;
    std::recursive_mutex                                handlers_mutex_;
    std::queue<patch>                                   patches_;
    std::recursive_mutex                                patches_mutex_;
    beast::websocket::close_code                        close_code_;

private:
    NON_MOVABLE_OR_COPYABLE(ws_conn);

public:
    ws_conn(asio::ip::tcp::socket &socket, std::shared_ptr<entity const> player_entity);

    void                            start();
    void                            write(std::shared_ptr<std::string const> msg);
    void                            close();
    bool                            pop_patch(patch &out);
    socket_t&                       socket();
    bool                            is_closed() const;
    std::shared_ptr<entity const>   player_entity() const;
    state                           current_state() const;

private:
    void write_next();
    void on_accept(boost::system::error_code const& ec) noexcept;
    void on_read(boost::system::error_code const& error, std::size_t const& bytes_transferred) noexcept;
    void on_write(boost::system::error_code const& ec, std::size_t const& bytes_transferred) noexcept;
    void on_close() noexcept;
    void do_read();
    void do_close(beast::websocket::close_code const& code);
    void push_patch(std::string const& what, any const& value);
    void push_patch(patch const& p);
};
