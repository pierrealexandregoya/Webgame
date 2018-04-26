#include <functional>
#include <set>
#include <algorithm>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "WsConn.hpp"
#include "Entity.hpp"
#include "Server.hpp"
#include "Behavior.hpp"

#define                                 TS (1000.f / 20)

Server::Server(unsigned int port)
    : timeSpan_(static_cast<int>(TS))
    , io_context_()
    , acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port))
    , t_(io_context_, timeSpan_)
    , socket_(io_context_)
{
    // First NPC
    addEntity(Entity({ 0.5f, 0.5f }, { -0.1f, -0.1f }, "npc1", {new Walk()}));

    // 100 static objects centered at 0,0
    const int s = 10;
    for (int i = 0; i < s; ++i)
        for (int j = 0; j < s; ++j)
            addEntity(Entity({ i - s / 2.f , j - s / 2.f }, { 0, 0 }, "object1"));
}

void Server::start_accept()
{
    acceptor_.async_accept(socket_, boost::bind(&Server::on_accept, this, boost::asio::placeholders::error));
}

void Server::on_accept(const boost::system::error_code& ec)
{
    if (ec)
    {
        std::cerr << "ACCEPT ERROR: " << ec.message() << std::endl;;
        return;
    }
    
    Entity playerEntity({ 0, 0 }, { 0, 0 }, "player");
    auto pair = entities_.insert(std::make_pair(playerEntity.id(), playerEntity));
    assert(pair.second == true);
    auto new_connection = WsConn::pointer(new WsConn(socket_, pair.first->second));
    conns_.insert(std::make_pair(pair.first->second.id(), new_connection));
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

void fail(boost::system::error_code ec, char const* what)
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

boost::property_tree::ptree Server::entityToPtree(Entity const& e)
{
    boost::property_tree::ptree ptEntity;
    ptEntity.put("id", e.id());
    ptEntity.put("type", e.type());
    ptEntity.add_child("pos", vecToPtree(e.pos()));
    ptEntity.add_child("vel", vecToPtree(e.vel()));
    return ptEntity;
}

std::string Server::entitiesToJson()
{
    boost::property_tree::ptree root;
    root.put("order", "state");
    root.put("suborder", "entities");
    boost::property_tree::ptree data;
    for (auto &p : entities_)
    {
        auto e = p.second;
        boost::property_tree::ptree ptEntity = entityToPtree(e);
        data.push_back(std::make_pair("", ptEntity));
    }
    root.add_child("data", data);
    std::stringstream ss;
    boost::property_tree::write_json(ss, root);
    return ss.str();
}

void Server::loop(const boost::system::error_code& /*e*/)
{
    static auto now = std::chrono::steady_clock::now();
    auto newNow = std::chrono::steady_clock::now();
    float d = (newNow - now).count() / 1000000000.;
    //std::cout << "LOOP " << d << std::endl;
    now = newNow;
    //// If a player got disconnected, we remove the corresponding connection and entity objects
    std::list<int> idsToRemove;
    for (auto const& p : conns_)
        if (p.second->isClosed())
            idsToRemove.push_back(p.first);
    for (auto id : idsToRemove)
    {
        assert(entities_[id].type() == "player");
        std::cout << "Removing id " << id << " from connections and entities" << std::endl;
        if (conns_.erase(id) == 0)
            std::cout << "No id in connections" << std::endl;
        if (entities_.erase(id) == 0)
            std::cout << "No id in entities" << std::endl;
    }

    std::string removeMsg = "";
    if (!idsToRemove.empty())
    {
        boost::property_tree::ptree root;
        root.put("order", "remove");
        root.put("suborder", "entities");

        boost::property_tree::ptree ids;
        for (auto &id : idsToRemove)
        {
            boost::property_tree::ptree pt;
            pt.put("", id);
            ids.push_back(std::make_pair("", pt));
        }
        root.add_child("ids", ids);

        std::stringstream ss;
        boost::property_tree::write_json(ss, root);
        removeMsg = ss.str();
    }
        
    // Update all entities with delta
    for (auto &p : entities_)
        p.second.update(d/*TS / 1000*/); // FIXME: proper delta

    // Serialize all entities
    std::string jsonEntities = entitiesToJson();
    for (auto &c : Server::conns_)
    {
        if (removeMsg != "")
            c.second->write(removeMsg);
        c.second->write(jsonEntities);
    }

    // Reset timer
    t_.expires_after(timeSpan_);
    t_.async_wait(boost::bind(&Server::loop, this, _1));
}

void Server::addEntity(Entity &&e)
{
    entities_.insert(std::make_pair(e.id(), std::move(e)));
}
