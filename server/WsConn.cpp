#include <ctype.h>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <numeric>

#include "WsConn.hpp"
#include "Server.hpp"
#include "Entity.hpp"

void WsConn::start()
{
    std::cout << addrStr_ << " " << "HANDSHAKING" << std::endl;
    socket_.async_accept(boost::asio::bind_executor(strand_, std::bind(&WsConn::on_accept, shared_from_this(), std::placeholders::_1)));
}

//WsConn::pointer WsConn::create(boost::asio::ip::tcp::socket &socket, /*shared_ptr?*/Entity &entity)
//{
//    return pointer(new WsConn(socket, entity));
//}

WsConn::WsConn(boost::asio::ip::tcp::socket &socket, /* shared_ptr? */ Entity &entity)
    : addrStr_("[" + socket.remote_endpoint().address().to_string() + "]:" + std::to_string(socket.remote_endpoint().port()))
    , socket_(std::move(socket))
    , strand_(socket_.get_executor())
    , isWriting(false)
    , playerEntity_(entity)
    , closed_(false)
{
    std::cout << addrStr_ << " " << "CONNECTED" << std::endl;
}

WsConn::SocketType& WsConn::socket()
{
    return socket_;
}

bool WsConn::isClosed() const
{
    return closed_;
}

Entity const& WsConn::playerEntity() const
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

void WsConn::on_close(const boost::system::error_code& ec)
{
  std::cout << addrStr_ << " SESSION CLOSED" << (ec ? ": " + ec.message() : "") << std::endl;
  closed_ = true;
}

void WsConn::do_read()
{
    std::cout << addrStr_ << " " << "READING" << std::endl;
    std::istringstream issTmp;
    socket_.async_read(readBuf_, boost::bind(&WsConn::on_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void WsConn::on_read(const boost::system::error_code& ec, size_t bytes_transferred)
{
    if (ec == boost::beast::websocket::error::closed)
    {
        std::cout << addrStr_ << " READ: SESSION CLOSED" << std::endl;
// 	socket_.async_close(boost::beast::websocket::close_code::normal, 
// 	[&](const boost::system::error_code& ec) {
// -            std::cout << addrStr_ << " SESSION CLOSED" << (ec ? ": " + ec.message() : "") << std::endl;
// -            closed_ = true;
// -        });
//         return;
    }
    if (ec)
    {
        std::cout << addrStr_ << " READ ERROR: " << ec.message() << std::endl;
        // socket_.async_close(boost::beast::websocket::close_code::abnormal, on_close);
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
                playerEntity_.setVel(Array2({ ptree.get<float>("vel.x"), ptree.get<float>("vel.y") }));
                if (fabsf(boost::numeric::ublas::norm_2(playerEntity_.vel())) > std::numeric_limits<float>::epsilon())
                    playerEntity_.setVel(VecType(playerEntity_.vel() / boost::numeric::ublas::norm_2(playerEntity_.vel())));
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
    /*socket_.text(socket_.got_text());
    socket_.async_write(readBuf_.data(), boost::asio::bind_executor(strand_, std::bind(&WsConn::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2)));*/
}

//void WsConn::on_write(boost::system::error_code ec, std::size_t bytes_transferred)
//{
//    if (ec)
//    {
//        std::cerr << "WsConn::on_write" << ": " << ec.message() << "\n";
//        return;
//    }
//
//    // Clear the buffer
//    readBuf_.consume(readBuf_.size());
//
//    // Do another read
//    do_read();
//}
