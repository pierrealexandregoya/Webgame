#include "server.hpp"

#include <cassert>
#include <future>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "behavior.hpp"
#include "entities.hpp"
#include "entity.hpp"
#include "env.hpp"
#include "lock.hpp"
#include "log.hpp"
#include "npc.hpp"
#include "persistence.hpp"
#include "player.hpp"
#include "player_conn.hpp"
#include "protocol.hpp"
#include "save_load.hpp"
#include "stationnary_entity.hpp"
#include "time.hpp"
#include "utils.hpp"
#include "vector.hpp"

using namespace std::literals::chrono_literals;

namespace asio = boost::asio;
namespace beast = boost::beast;

namespace webgame {

server::server(asio::io_context &io_context, unsigned int port, std::shared_ptr<persistence> const& persistence, std::string const& game_name, std::function<std::shared_ptr<player>(entities &ents)> &&init_player)
    : io_context_(io_context)
    , local_endpoint_(asio::ip::tcp::endpoint(asio::ip::tcp::v6(), port))
    , acceptor_(io_context_)
    , new_client_socket_(io_context_)
    , persistence_(persistence)
    , stop_(new bool(false))
    , game_cycle_timer_(io_context)
    , init_player_(std::move(init_player))
    , game_name_(game_name)
{}

void server::shutdown()
{
    WEBGAME_LOCK(server_mutex_);

    WEBGAME_LOG("SHUTDOWN", "Stopping game loop thread");
    *stop_ = true;
    game_cycle_timer_.cancel();

    WEBGAME_LOG("SHUTDOWN", "Cancelling server accept");
    acceptor_.cancel();

    for (auto conn : conns_)
    {
        WEBGAME_LOG("SHUTDOWN", "Closing connection " << conn->addr_str);
        conn->close();
    }
    conns_.clear();

    entities_.clear();

    WEBGAME_LOG("SHUTDOWN", "Closing server socket");
    acceptor_.close();

    WEBGAME_LOG("SHUTDOWN", "Stopping persistence instance");
    persistence_->stop();
}

bool server::is_player_connected(std::string const& name)
{
    WEBGAME_LOCK(server_mutex_);

    for (std::shared_ptr<player_conn> const& conn : conns_)
        if (conn->current_state() > player_conn::authenticating
            && conn->current_state() < player_conn::closing
            && conn->player_name() == name)
            return true;
    return false;
}

void server::async_load_player(std::shared_ptr<player_conn> const& conn, std::string const& name, std::function<load_player_handler> &&handler)
{
    auto this_p = shared_from_this();
    auto handler_p = std::make_shared<std::function<load_player_handler>>(std::move(handler));

    persistence_->async_check_player(name, [this_p, conn, name, handler_p](bool success, id_t id) {
        if (!success)
        {
            WEBGAME_LOCK(this_p->server_mutex_);
            std::shared_ptr<player> player = this_p->init_player_(this_p->entities_);
            this_p->persistence_->async_add_player(name, player, [this_p, conn, handler_p, player] {
                //(*handler_p)(player);
                this_p->register_player(conn, player);
            });
        }
        else
        {
            this_p->persistence_->async_get_player(id, [this_p, conn, name, handler_p](bool success, std::shared_ptr<player> const& player) {
                if (!success)
                {
                    WEBGAME_LOG("SERVER", "ERROR: player name " << name << " exists but not its datas. Closing connection...");
                    conn->close();
                }
                else
                {
                    //(*handler_p)(player);
                    this_p->register_player(conn, player);
                }
            });
        }
    });
}

void server::register_player(std::shared_ptr<player_conn> const& player_conn, std::shared_ptr<player> const& player_ent)
{
    WEBGAME_LOCK(server_mutex_);

    player_conn->set_player_entity(player_ent);
    player_ent->set_conn(&(*player_conn));

    player_conn->write(std::make_shared<std::string const>("{\"order\":\"state\",\"suborder\":\"game\",\"tick_duration\":"
        + std::to_string(std::chrono::duration_cast<std::chrono::duration<float>>(tick_duration_).count())
		+ ",\"game_name\":\"" + game_name_ +"\""+ "}"));
    player_conn->write(std::make_shared<std::string const>(json_state_player(player_ent)));

    player_conn->write(std::make_shared<std::string const>(json_state_entities(entities_)));

    connections other_conns;
    for (auto &other_conn : conns_)
        if (other_conn != player_conn && other_conn->is_ready())
            other_conns.emplace_back(other_conn);

    if (!other_conns.empty())
    {
        auto new_entity_json = std::make_shared<std::string const>(json_state_entities(entities({ player_ent })));
        for (auto &other_conn : other_conns)
            other_conn->write(new_entity_json);
    }

    //player_conn->set_state(player_conn::reading);
    entities_.add(player_ent);

    WEBGAME_LOG("SERVER", "PLAYER " << player_conn->player_name() << " REGISTERED");
}

std::shared_ptr<persistence> server::get_persistence()
{
    return persistence_;
}

connections const& server::get_connections() const
{
    return conns_;
}

entities const& server::get_entities() const
{
    return entities_;
}

void server::start_persistence()
{
    WEBGAME_LOG("STARTUP", "LOADING WORLD");
    if (!persistence_->start())
        throw std::runtime_error("server: could not start persistence instance");

    entities_ = persistence_->load_all_npes();
    WEBGAME_LOG("STARTUP", "LOADED " << static_cast<entity_container<stationnary_entity>>(entities_).size() << " STATIONNARY ENTITIES");
    WEBGAME_LOG("STARTUP", "LOADED " << static_cast<entity_container<npc>>(entities_).size() << " CHARACTER ENTITIES");
}

void server::start_game()
{
    *stop_ = false;
    wake_time_ = std::chrono::ceil<std::chrono::seconds>(steady_clock::now());
#ifndef NDEBUG
    start_time_ = wake_time_;
#endif /* !NDEBUG */
    assert(std::chrono::duration_cast<std::chrono::nanoseconds>(wake_time_.time_since_epoch()).count() % 1000000000 == 0);

    game_cycle_timer_.expires_at(wake_time_);
    game_cycle_timer_.async_wait(std::bind(&server::game_cycle, shared_from_this(), std::placeholders::_1, 0));

    WEBGAME_LOG("STARTUP", "FIRST GAME CYCLE IN "
        << std::chrono::duration_cast<std::chrono::duration<float>>(wake_time_ - std::chrono::steady_clock::now()).count()
        << "s WITH TICK DURATION OF "
        << std::chrono::duration_cast<std::chrono::duration<float>>(tick_duration_).count() << "s");
}

void server::start_network()
{
    acceptor_.open(local_endpoint_.protocol());
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    acceptor_.bind(local_endpoint_);
    acceptor_.listen();

    new_client_socket_ = asio::ip::tcp::socket(io_context_);

    asio::post(acceptor_.get_executor(), std::bind(&server::do_accept, shared_from_this()));

    WEBGAME_LOG("STARTUP", "LISTENNING FOR PLAYER CONNECTION");
}

void server::game_cycle(boost::system::error_code const& error, unsigned int nb_ticks)
{
    WEBGAME_LOCK(server_mutex_);

    if (*stop_)
    {
        WEBGAME_LOG("GAME LOOP", "STOPPED");
        return;
    }

    double delta = (std::chrono::duration_cast<delta_duration>(tick_duration_) * nb_ticks).count();

    // If a player got disconnected, we remove the corresponding connection and entity objects
    std::list<decltype(conns_)::const_iterator> its_to_remove;
    for (auto it = conns_.cbegin(); it != conns_.cend(); ++it)
        if (/*(*it)->is_closed()*/(*it)->current_state() > player_conn::to_be_closed)
            its_to_remove.push_back(it);

    std::list<id_t> ids_to_remove;
    for (auto it : its_to_remove)
    {
        if ((*it)->player_entity())
        {
            id_t id = (*it)->player_entity()->id();

            ids_to_remove.push_back(id);

            assert(entities_.count(id) == 1);
            //assert(entities_.at(id)->type() == "player");

            WEBGAME_LOG("GAME LOOP", "Removing id " << id << " from entities");

            entities_.erase(id);
        }
        WEBGAME_LOG("GAME LOOP", "Removing conn " << (*it)->addr_str << " from connections");
        conns_.erase(it);
    }

    // Fixme: should have a helper in json.cpp
    // We build the remove message
    std::shared_ptr<std::string const> remove_entities_msg;
    if (!ids_to_remove.empty())
    {
        boost::property_tree::ptree root;
        root.put("order", "remove");
        root.put("suborder", "entities");

        boost::property_tree::ptree ids;
        for (auto &id : ids_to_remove)
        {
            boost::property_tree::ptree pt;
            pt.put("", id);
            ids.push_back(std::make_pair("", pt));
        }
        root.add_child("ids", ids);

        std::stringstream ss;
        boost::property_tree::write_json(ss, root, false);
        remove_entities_msg = std::make_shared<std::string const>(std::move(ss.str()));
    }

    //// Apply all pending patches of all connections
    //for (auto &c : conns_)
    //{
    //    if (!c->is_ready())
    //        continue;

    //    while (c->has_patch())
    //    {
    //        player_conn::patch p = c->pop_patch();;

    //        try {
    //            if (p.what == "speed")
    //                c->player_entity()->set_speed(any_cast<double>(p.value));
    //            else if (p.what == "dir")
    //            {
    //                c->player_entity()->set_dir(any_cast<vector>(p.value));
    //                c->player_entity()->stop();
    //            }
    //            else if (p.what == "target_pos")
    //                c->player_entity()->move_to(any_cast<vector>(p.value));
    //            else
    //                WEBGAME_LOG("GAME LOOP", "ERROR during patches application: unknown value: " << p.what);
    //        }
    //        catch (const bad_any_cast& e) {
    //            WEBGAME_LOG("GAME LOOP", "ERROR during patches application: " << e.what() << ". Conn: " << c->addr_str << ", entity: " << c->player_entity()->id() << ", value name: " << p.what);
    //        }
    //    }
    //}

    entities alive_entities;
    for (auto &ent : entities_)
    {
        std::shared_ptr<player> player_p = std::dynamic_pointer_cast<player>(ent.second);
        if (player_p && !player_p->conn()->is_ready())
            continue;

        alive_entities.add(ent.second);
    }

    entities changed_entities;
    for (auto &ent : alive_entities)
    {
        env env(alive_entities);
        // Remove the entity from its own env so it doesn't see itself
        env.others().erase(ent.second->id());
        if (ent.second->update(delta, env))
            changed_entities.add(ent.second);
    }

    // Save to redis, fixme: maybe just save alive entities
    persistence_->async_save(entities_, [] {
        //LOG("SERVER", "ENTITIES SAVED");
    });

    // We broadcast all changes to all players
    if (remove_entities_msg)
        for (auto &c : server::conns_)
            if (c->is_ready())
                c->write(remove_entities_msg);

    if (!changed_entities.empty())
    {
        std::shared_ptr<std::string const> state_entities_msg = std::make_shared<std::string const>(json_state_entities(changed_entities));

        for (auto &c : server::conns_)
            if (c->is_ready())
            {
                entities entities_for_player = changed_entities;
                if (entities_for_player.erase(c->player_entity()->id()) == 0)
                    c->write(state_entities_msg);
                else if (!entities_for_player.empty())
                    c->write(std::make_shared<std::string const>(json_state_entities(entities_for_player)));
            }
    }

    for (auto &c : server::conns_)
        if (c->is_ready())
            c->write(std::make_shared<std::string>(json_state_player(c->player_entity())));

    if (*stop_)
    {
        WEBGAME_LOG("GAME LOOP", "STOPPED");
        return;
    }

    // Prepare next call
    nb_ticks = 0;
    while (wake_time_ < steady_clock::now())
    {
        wake_time_ += tick_duration_;
        ++nb_ticks;
    }
    assert(nb_ticks >= 1);

    if (nb_ticks > 1)
        WEBGAME_LOG("GAME LOOP", "RETARD OF " << nb_ticks - 1 << " TICK" << (nb_ticks - 1 > 1 ? "S" : ""));

    //LOG("GAME LOOP", "NEXT CYCLE AT " << std::chrono::duration_cast<std::chrono::duration<float>>(wake_time_ - start_time_).count());
    game_cycle_timer_.expires_at(wake_time_);
    game_cycle_timer_.async_wait(std::bind(&server::game_cycle, shared_from_this(), std::placeholders::_1, nb_ticks));
}

void server::on_accept(const boost::system::error_code& ec) noexcept
{
    if (ec)
    {
        if (ec.value() == 995)
            WEBGAME_LOG("SERVER ACCEPT", "INFO: operation aborted");
        else
            WEBGAME_LOG("SERVER ACCEPT", "ERROR " << ec.value() << ": " << ec.message());

        boost::system::error_code ec;
        new_client_socket_.close(ec);

        if (!*stop_)
            do_accept();
        return;
    }

    WEBGAME_LOCK(server_mutex_);

    // socket can be invalid even if there is no error, calling remote_endpoint() is a way to test it
    try {
        std::string const endpoint = new_client_socket_.remote_endpoint().address().to_string() + ":" + std::to_string(new_client_socket_.remote_endpoint().port());
        WEBGAME_LOG("SERVER ACCEPT", "New connection from: " << endpoint);
    }
    catch (...) {
        WEBGAME_LOG("SERVER ACCEPT", "ERROR while creating new player entity, new connection or while starting it");
        boost::system::error_code ec;
        new_client_socket_.close(ec);
    }

    auto new_conn = std::make_shared<player_conn>(std::move(new_client_socket_), io_context_, shared_from_this());

    conns_.emplace_back(new_conn);

    new_conn->start();

    do_accept();
}

void server::do_accept()
{
    acceptor_.async_accept(new_client_socket_, std::bind(&server::on_accept, shared_from_this(), std::placeholders::_1));
}

} // namespace webgame
