#include <boost/bind.hpp>

#include "WsConn.hpp"

void WsConn::start()
{
    socket_.async_accept(boost::asio::bind_executor(strand_, std::bind(&WsConn::on_accept, shared_from_this(), std::placeholders::_1)));
}

WsConn::pointer WsConn::create(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket &socket)
{
    return pointer(new WsConn(io_context, socket));
}

WsConn::SocketType& WsConn::socket()
{
    return socket_;
}

void WsConn::write(std::string const& msg)
{
    size_t i = 0;
    auto bufs = buffer_.prepare(msg.size());
    for (auto const& b : bufs) {
        memcpy(b.data(), &msg[i], b.size());
        i += b.size();
    }
    toWrite_.push(bufs);
    writeNext();
}

WsConn::WsConn(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket &socket)
    : socket_(std::move(socket))
    , strand_(socket_.get_executor())
    , isWriting(false)
{}

void WsConn::writeNext()
{
    if (toWrite_.empty() || isWriting)
        return;
    auto bufs = toWrite_.front();
    toWrite_.pop();
    socket_.async_write(bufs, boost::asio::bind_executor(strand_, [&](boost::system::error_code e, std::size_t bytes_transferred) {
        //std::cout << bytes_transferred << std::endl;
        isWriting = false;
        writeNext();
    }));
    isWriting = true;
}

void WsConn::on_accept(boost::system::error_code ec)
{
    if (ec)
    {
        std::cerr << "WsConn::on_accept" << ": " << ec.message() << "\n";
        return;
    }

    //do_read();
}

void WsConn::do_read()
{
    socket_.async_read(readBuf_, boost::bind(&WsConn::on_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void WsConn::on_read(const boost::system::error_code& error, size_t bytes_transferred)
{
    socket_.text(socket_.got_text());
    socket_.async_write(readBuf_.data(), boost::asio::bind_executor(strand_, std::bind(&WsConn::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2)));
}

void WsConn::on_write(boost::system::error_code ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        std::cerr << "WsConn::on_write" << ": " << ec.message() << "\n";
        return;
    }

    // Clear the buffer
    readBuf_.consume(readBuf_.size());

    // Do another read
    do_read();
}
