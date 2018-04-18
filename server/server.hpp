#pragma once

#include <array>
#include <list>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/system/error_code.hpp>
#include <boost/property_tree/ptree.hpp>

#include "WsConn.hpp"
//#include "Entity.hpp"

class Entity;

typedef boost::numeric::ublas::vector<float> VecType;
typedef std::array<float, 3> Array2;

class Server
{
private:
    boost::asio::io_context             io_context_;

    boost::asio::ip::tcp::acceptor      acceptor_;
    std::list<WsConn::pointer>          conns_;
#define                                 TS (1000.f / 60)
    boost::asio::chrono::milliseconds   timeSpan_;
    std::map<int, Entity>               entities_;
    boost::asio::steady_timer           t_;

public:
    static void fail(boost::system::error_code ec, char const* what);
    static boost::property_tree::ptree vecToPtree(VecType const& v);

    Server(unsigned int port);

    void run();
    void loop(const boost::system::error_code& /*e*/);

private:
    void start_accept();
    void on_accept(boost::asio::ip::tcp::socket *socket, const boost::system::error_code& error);
};
