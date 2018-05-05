#include "WsConn.hpp"

#include <ctype.h>
#include <numeric>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "Entity.hpp"
#include "Server.hpp"
#include "types.hpp"

WsConn::WsConn(boost::asio::ip::tcp::socket &socket, P<Entity> const& entity)
    : addrStr_("[" + socket.remote_endpoint().address().to_string() + "]:" + std::to_string(socket.remote_endpoint().port()))
    , socket_(std::move(socket))
    , strand_(socket_.get_executor())
    , isWriting(false)
    , playerEntity_(entity)
    , closed_(false)
{
    std::cout << addrStr_ << " " << "CONNECTED" << std::endl;
}

void WsConn::start()
{
    std::cout << addrStr_ << " " << "HANDSHAKING" << std::endl;
    std::cout << shared_from_this().use_count() << std::endl;
    std::cout << shared_from_this().use_count() << std::endl;
    socket_.async_accept(boost::asio::bind_executor(strand_, std::bind(&WsConn::on_accept, shared_from_this(), std::placeholders::_1)));
}

WsConn::SocketType& WsConn::socket()
{
    return socket_;
}

bool WsConn::isClosed() const
{
    return closed_;
}

P<Entity> const& WsConn::playerEntity() const
{
    return playerEntity_;
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

void WsConn::writeNext()
{
    if (toWrite_.empty() || isWriting)
        return;
    auto bufs = toWrite_.front();
    toWrite_.pop();
    socket_.text(true);
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
    std::cout << addrStr_ << " " << "            DONE" << std::endl;
    boost::property_tree::ptree root = Server::entityToPtree(playerEntity_);
    root.put("order", "state");
    root.put("suborder", "player");
    std::stringstream ss;
    boost::property_tree::write_json(ss, root);
    write(ss.str());
    do_read();
}

void WsConn::on_close()
{
    std::cout << addrStr_ << " SESSION CLOSED" << std::endl;
    closed_ = true;
}

void WsConn::do_read()
{
    std::cout << addrStr_ << " READING" << std::endl;
    socket_.async_read(readBuf_, boost::bind(&WsConn::on_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void WsConn::on_read(const boost::system::error_code& ec, size_t bytes_transferred)
{
    if (ec == boost::beast::websocket::error::closed)
    {
        std::cout << addrStr_ << " READ: SESSION CLOSED" << std::endl;
        socket_.async_close(boost::beast::websocket::close_code::normal, std::bind(&WsConn::on_close, shared_from_this()));
    }
    if (ec)
    {
        std::cout << addrStr_ << " READ ERROR: " << ec.message() << std::endl;
         socket_.async_close(boost::beast::websocket::close_code::abnormal, std::bind(&WsConn::on_close, shared_from_this()));
        return;
    }
    std::cout << addrStr_ << " READ " << bytes_transferred << " BYTES:"/* << std::endl*/;

    for (auto const& b : readBuf_.data())
        for (int i = 0; i < b.size(); ++i)
        {
            char c = static_cast<char const*>(b.data())[i];
            if (isprint(c))
                if (c == '\\')
                    std::cout << "\\\\";
                else
                    std::cout << c;
            else
                std::cout << "\0x" << std::hex
                << static_cast<int>(static_cast<unsigned char>(c));
        }
    //std::cout << boost::beast::buffers(readBuf_.data()) << std::endl;
    std::cout << std::endl;
    std::string str;
    for (auto const& buf : readBuf_.data())
        str += std::string(static_cast<char const *>(buf.data()), buf.size());
    readBuf_.consume(readBuf_.size());
    std::istringstream iss(str);
    boost::property_tree::ptree ptree;
    try {
        boost::property_tree::read_json(iss, ptree);


        std::string order = ptree.get<std::string>("order");
        if (order == "state")
        {
            std::string suborder = ptree.get<std::string>("suborder");
            if (suborder == "player")
            {
                playerEntity_->setDir({ ptree.get<float>("vel.x"), ptree.get<float>("vel.y") });
                if (fabsf(boost::numeric::ublas::norm_2(playerEntity_->dir())) > std::numeric_limits<float>::epsilon())
                    playerEntity_->setDir(playerEntity_->dir() / boost::numeric::ublas::norm_2(playerEntity_->dir()));
            }
        }
    }
    catch (boost::property_tree::json_parser_error const& e) {
        std::cerr << addrStr_ << "READ: JSON PARSER ERROR: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << addrStr_ << "READ: EXCEPTION THROWN" << std::endl;
    }
    do_read();
}

