#pragma once

#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/enable_shared_from_this.hpp>

class Entity;

class WsConn : public boost::enable_shared_from_this<WsConn>
{
public:
    typedef boost::shared_ptr<WsConn>                                       pointer;
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
    Entity                                                          &playerEntity_;
    bool                                                            closed_;

public:
    WsConn(boost::asio::ip::tcp::socket &socket, Entity &playerEntity);

    void            start();
    void            write(std::string const& msg);
    SocketType&     socket();
    bool            isClosed() const;
    Entity const&   playerEntity() const;

private:
    void writeNext();
    void on_accept(boost::system::error_code ec);
    void on_close();
    void do_read();
    void on_read(const boost::system::error_code& error, size_t bytes_transferred);
    //void on_write(boost::system::error_code ec, std::size_t bytes_transferred);
};
