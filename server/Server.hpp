#pragma once

#include <string>
#include <array>
#include <list>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/system/error_code.hpp>
#include <boost/property_tree/ptree.hpp>

#include "WsConn.hpp"

typedef boost::numeric::ublas::vector<float> VecType;
typedef std::array<float, 3> Array2;

#include "Entity.hpp"

class Server
{
private:
    boost::asio::io_context             io_context_;

    boost::asio::ip::tcp::acceptor      acceptor_;
    std::map<int, WsConn::pointer>      conns_;
    boost::asio::chrono::milliseconds   timeSpan_;
    std::map<int, Entity>               entities_;
    boost::asio::steady_timer           t_;
    boost::asio::ip::tcp::socket        socket_;

public:
    static boost::property_tree::ptree vecToPtree(VecType const& v);

    Server(unsigned int port);

    void run();
    static boost::property_tree::ptree entityToPtree(Entity const& e);
    std::string entitiesToJson();
    void loop(const boost::system::error_code& /*e*/);

private:
    void start_accept();
    void on_accept(const boost::system::error_code& error);
    void addEntity(Entity &&e);
};

//void fail(boost::system::error_code ec, char const* what);
