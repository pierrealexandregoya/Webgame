#pragma once

#include <queue>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "types.hpp"

class Entity;

class WsConn : public std::enable_shared_from_this<WsConn>
{
public:
    typedef boost::beast::websocket::stream<boost::asio::ip::tcp::socket>   SocketType;

private:
    std::string                                                     addrStr_;
    SocketType                                                      socket_;
    std::string                                                     message_;
    boost::beast::multi_buffer                                      readBuf_;
    boost::asio::strand<boost::asio::io_context::executor_type>     strand_;
    std::queue<boost::beast::multi_buffer::mutable_buffers_type>    toWrite_;
    boost::beast::multi_buffer                                      buffer_;
    bool                                                            isWriting;
    P<Entity>                                                       playerEntity_;
    bool                                                            closed_;

    NON_MOVABLE_OR_COPYABLE(WsConn);

public:
    WsConn(boost::asio::ip::tcp::socket &socket, P<Entity> const& playerEntity);

    void                            start();
    void                            write(std::string const& msg);
    SocketType&                     socket();
    bool                            isClosed() const;
    P<Entity> const&  playerEntity() const;

private:
    void writeNext();
    void on_accept(boost::system::error_code ec);
    void on_close();
    void do_read();
    void on_read(const boost::system::error_code& error, size_t bytes_transferred);
};
