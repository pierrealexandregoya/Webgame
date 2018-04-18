#pragma once

#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/enable_shared_from_this.hpp>

class WsConn : public boost::enable_shared_from_this<WsConn>
{
public:
    typedef boost::shared_ptr<WsConn>                                       pointer;
    typedef boost::beast::websocket::stream<boost::asio::ip::tcp::socket>   SocketType;

private:
    SocketType                                                      socket_;
    std::string                                                     message_;
    boost::beast::multi_buffer                                      readBuf_;
    boost::asio::strand<boost::asio::io_context::executor_type>     strand_;
    std::queue<boost::beast::multi_buffer::mutable_buffers_type>    toWrite_;
    boost::beast::multi_buffer                                      buffer_;
    bool                                                            isWriting;

public:
    static pointer  create(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket &socket);

    void            start();
    void            write(std::string const& msg);
    SocketType&     socket();

private:
    WsConn(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket &socket);

    void writeNext();
    void on_accept(boost::system::error_code ec);
    void do_read();
    void on_read(const boost::system::error_code& error, size_t bytes_transferred);
    void on_write(boost::system::error_code ec, std::size_t bytes_transferred);
};
