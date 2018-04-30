#include "Server.hpp"

#include <algorithm>
#include <functional>
#include <set>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "Behavior.hpp"
#include "Entity.hpp"
#include "WsConn.hpp"

#define                                 TS (1000.f / 20)

Server::Server(unsigned int port)
    : entities_(MP<Entities>())
    , io_context_()
    , acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port))
    , socket_(io_context_)
    , timeSpan_(static_cast<int>(TS))
    , t_(io_context_, timeSpan_)
{
    //// First NPC
    addEntity({ 0.5f, 0.5f }, { -0.1f, -0.1f }, 1.f, "npc1", Behaviors({ {1.f, MP<Walk>()}, {0.f, MP<AreaLimit>(AreaLimit::Square, 1)} }));

    //// 100 static objects centered at 0,0
    const int s = 10;
    for (int i = 0; i < s; ++i)
        for (int j = 0; j < s; ++j)
            addEntity({ i - s / 2.f , j - s / 2.f }, { 0, 0 }, 0.f, "object1");
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
        assert(entities_->at(id)->type() == "player");
        std::cout << "Removing id " << id << " from connections and entities" << std::endl;
        if (conns_.erase(id) == 0)
            std::cout << "No id in connections" << std::endl;
        if (entities_->erase(id) == 0)
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
    Env env(entities_);
    for (auto &p : *entities_)
        p.second->update(d, env);

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

void Server::run()
{
    start_accept();

    t_.async_wait(boost::bind(&Server::loop, this, _1));

    io_context_.run();
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

    auto e = addEntity({ 0, 0 }, { 0, 0 }, 1.f, "player");
    auto new_connection = MP<WsConn>(std::ref(socket_), e);
    std::cout << new_connection.use_count() << std::endl;
    //auto new_connection = std::make_shared<WsConn>(socket_, e);
    assert(conns_.insert({ e->id(), new_connection }).second);
    std::cout << new_connection.use_count() << std::endl;

    new_connection->start();

    start_accept();
}

boost::property_tree::ptree Server::vecToPtree(VecType const& v)
{
    boost::property_tree::ptree vecNode;
    vecNode.put("x", v[0]);
    vecNode.put("y", v[1]);
    return vecNode;
}

boost::property_tree::ptree Server::entityToPtree(P<Entity> const& e)
{
    boost::property_tree::ptree ptEntity;
    ptEntity.put("id", e->id());
    ptEntity.put("type", e->type());
    ptEntity.add_child("pos", vecToPtree(e->pos()));
    ptEntity.add_child("vel", vecToPtree(e->vel()));
    return ptEntity;
}

std::string Server::entitiesToJson()
{
    boost::property_tree::ptree root;
    root.put("order", "state");
    root.put("suborder", "entities");
    boost::property_tree::ptree data;
    for (auto &p : *entities_)
    {
        auto &e = p.second;
        boost::property_tree::ptree ptEntity = entityToPtree(e);
        data.push_back(std::make_pair("", ptEntity));
    }
    root.add_child("data", data);
    std::stringstream ss;
    boost::property_tree::write_json(ss, root);
    return ss.str();
}

P<Entity> Server::addEntity(Array2 const& pos, Array2 const& vel, float speed, std::string const& type, Behaviors && behaviors)
{
    P<Entity> e = std::make_shared<Entity>(pos, vel, speed, type, std::move(behaviors));
    auto r = entities_->insert(std::make_pair(e->id(), e));
    assert(r.second);
    return e;
}
