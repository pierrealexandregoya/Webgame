#include <functional>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "WsConn.hpp"
#include "Entity.hpp"
#include "server.hpp"

Server::Server(unsigned int port)
    : timeSpan_(static_cast<int>(TS))
    , io_context_()
    , acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port))
    , t_(io_context_, timeSpan_)
{
    Entity enemy1 = Entity({ 0.5f, 0.5f }, { -0.1f, -0.1f }, "enemy1");
    entities_.insert(std::make_pair(enemy1.id(), enemy1));

    const int s = 10;
    for (int i = 0; i < s; ++i)
        for (int j = 0; j < s; ++j)
        {
            Entity object = Entity({ i - s / 2.f , j - s / 2.f }, { 0, 0 }, "object1");
            entities_.insert(std::make_pair(object.id(), object));
        }
}

void Server::start_accept()
{
    boost::asio::ip::tcp::socket *socket = new boost::asio::ip::tcp::socket(acceptor_.get_executor().context());
    acceptor_.async_accept(*socket, boost::bind(&Server::on_accept, this, socket, boost::asio::placeholders::error));
}

void Server::on_accept(boost::asio::ip::tcp::socket *socket,
    const boost::system::error_code& error)
{
    if (error)
        return Server::fail(error, "TcpServer::on_accept");

    WsConn::pointer new_connection = WsConn::create(acceptor_.get_executor().context(), *socket);
    delete socket;
    conns_.push_back(new_connection);
    new_connection->start();

    start_accept();
}

void Server::run()
{
    try
    {
        start_accept();

        t_.async_wait(boost::bind(&Server::loop, this, _1));

        io_context_.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void Server::fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

boost::property_tree::ptree Server::vecToPtree(VecType const& v)
{
    boost::property_tree::ptree vecNode;
    vecNode.put("x", v[0]);
    vecNode.put("y", v[1]);
    return vecNode;
}

void Server::loop(const boost::system::error_code& /*e*/)
{
    for (auto &p : entities_)
        p.second.update(TS / 1000);

    boost::property_tree::ptree root;
    root.put("order", "state");
    root.put("suborder", "entities");
    boost::property_tree::ptree data;
    for (auto &p : entities_)
    {
        auto e = p.second;
        boost::property_tree::ptree ptEntity;
        ptEntity.put("id", e.id());
        ptEntity.put("type", e.type());
        ptEntity.add_child("pos", vecToPtree(e.pos()));
        ptEntity.add_child("vel", vecToPtree(e.vel()));
        data.push_back(std::make_pair("", ptEntity));
    }
    root.add_child("data", data);
    std::stringstream ss;
    boost::property_tree::write_json(ss, root);
    for (auto &c : Server::conns_)
        c->write(ss.str());
    t_.expires_after(timeSpan_);
    t_.async_wait(boost::bind(&Server::loop, this, _1));
}

int main()
{
    Server server(2000);
    server.run();

    return 0;
}
