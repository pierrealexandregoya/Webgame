#pragma once

#include <array>
#include <list>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "types.hpp"

class WsConn;
class Entity;

class Server
{
private:
    Connections                             conns_;
    P<Entities>                             entities_;

    boost::asio::io_context                 io_context_;
    boost::asio::ip::tcp::acceptor          acceptor_;
    boost::asio::ip::tcp::socket            socket_;

    boost::asio::chrono::milliseconds       timeSpan_;
    boost::asio::steady_timer               t_;

    NON_MOVABLE_OR_COPYABLE(Server);

public:
    static boost::property_tree::ptree vecToPtree(VecType const& v);

    Server(unsigned int port);

    void                                run();
    void                                loop(const boost::system::error_code& e);
    static boost::property_tree::ptree  entityToPtree(P<Entity> const& e);
    std::string                         entitiesToJson();
private:
    void        start_accept();
    void        on_accept(const boost::system::error_code& error);
    P<Entity>   addEntity(Array2 const& pos, Array2 const& vel, float speed, std::string const& type, Behaviors && behaviors=Behaviors());
};
