#include "server.hpp"

#include <cassert>

#include <future>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "behavior.hpp"
#include "entity.hpp"
#include "env.hpp"
#include "misc/json.hpp"
#include "misc/log.hpp"
#include "misc/time.hpp"
#include "misc/vector.hpp"
#include "ws_conn.hpp"

using namespace std::literals::chrono_literals;

namespace beast = boost::beast;

server::server(unsigned int port, unsigned int threads)
    : threads_(std::max<unsigned int>(1, threads))
    , io_context_(threads_)
    , acceptor_(io_context_, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), port))
    , new_client_socket_(io_context_)
    , tick_duration_(std::chrono::duration_cast<std::remove_const_t<decltype(tick_duration_)>>(0.250s))
    , stop_(new bool(false))
{
    // First ally NPC
    add_entity({ 0.5f, 0.5f }, { 0.f, 0.f }, 0.2f, 0.2f, "npc_ally_1", behaviors({
        { 0.f, std::make_shared<arealimit>(arealimit::Square, 0.5f, vector({ 0.5f, 0.5f }))} ,
        {1.f, std::make_shared<walkaround>()},
        }));

    // First enemy NPC
    add_entity({ -0.5f, -0.5f }, { 0.f, 0.f }, 0.4f, 0.4f, "npc_enemy_1", behaviors({
        { 0.f, std::make_shared<arealimit>(arealimit::Square, 0.5f, vector({ -0.5f, -0.5f })) } ,
        { 0.5f, std::make_shared<attack_on_sight>(0.7f) },
        { 1.f, std::make_shared<stop>() },
        //{ 1.f, std::make_shared<patrol>({,}, {,}, ...) },
        }));

    // 100 static objects centered at 0,0
    const int s = 10;
    for (int i = 0; i < s; ++i)
        for (int j = 0; j < s; ++j)
            add_entity({ i - s / 2.f , j - s / 2.f }, { 0.f, 0.f }, 0.f, 0.f, "object1");
}

void show_help()
{
    _MY_LOG("commands:" << std::endl
        << "\thelp" << std::endl
        << "\tinfo" << std::endl
        << "\texit" << std::endl
        << "\tio: set/unset io log: " << io_log << std::endl
        << "\tdata: set/unset data log (no effect if io log is unset): " << data_log
    );
}

void server::run()
{
    wake_time_ = std::chrono::ceil<std::chrono::seconds>(steady_clock::now());
    assert(std::chrono::duration_cast<std::chrono::nanoseconds>(wake_time_.time_since_epoch()).count() % 1000000000 == 0);

    // Run game loop
    auto loop_status = std::async(std::launch::async, [&] {

        std::this_thread::sleep_until(wake_time_);

        LOG("INFO", "GAME LOOP THREAD STARTED");

        while (!*stop_)
        {
            auto nb_ticks = 0;
            while (wake_time_ < steady_clock::now())
            {
                wake_time_ += tick_duration_;
                ++nb_ticks;
            }
            assert(nb_ticks >= 1);

            if (nb_ticks > 1)
                LOG("LOOP THREAD", "RETARD OF " << nb_ticks - 1 << " TICK" << (nb_ticks - 1 > 1 ? "S" : ""));
            std::this_thread::sleep_until(wake_time_);

            loop(nb_ticks);
        }

        LOG("INFO", "GAME LOOP THREAD STOPPED");
    });

    // Run network
    asio::post(acceptor_.get_executor(), std::bind(&server::do_accept, shared_from_this()));

    std::vector<std::thread> v;
    v.reserve(threads_);
    for (auto i = 0; i < threads_; ++i)
        v.emplace_back([&] {
        LOG("INFO", "NETWORK ASIO THREAD STARTED");
        try {
            io_context_.run();
        }
        catch (...) {
            assert(false);
        }
        assert(*stop_);
        LOG("INFO", "NETWORK ASIO THREAD STOPPED");
    });

    // Read user input
    std::string input;
    while (input != "exit")
    {
        std::cin >> input;
        if (input == "info")
        {
            LOG("INFO", "Connections: " << conns_.size());
            LOG("INFO", "Entities: " << entities_.size());
            LOG("INFO", "Threads: " << v.size() << " asio thread" << (v.size() > 1 ? "s" : "") << " + user input thread + game loop thread");
        }
        else if (input == "help")
            show_help();
        else if (input == "data")
            data_log = !data_log;
        else if (input == "io")
            io_log = !io_log;
        else
        {
            std::lock_guard<decltype(log_mutex)> lock(log_mutex);
            _MY_LOG("Unknown command: " << input << std::endl);
            show_help();
        }
    }

    // Server shutdown
    *stop_ = true;

    LOG("SERVER SHUTDOWN", "Stopping game loop thread");
    loop_status.wait();

    LOG("SERVER SHUTDOWN", "Cancelling server accept");
    acceptor_.cancel();
    for (auto conn : conns_)
    {
        LOG("SERVER SHUTDOWN", "Closing connection " << conn.second->addr_str);
        conn.second->close();
    }

    LOG("SERVER SHUTDOWN", "Closing server socket");
    acceptor_.close();

    for (auto & t : v)
    {
        LOG("SERVER SHUTDOWN", "Joining network thread " << t.get_id());
        t.join();
    }
}

void server::loop(unsigned int nb_ticks)
{
    std::lock_guard<decltype(conns_mutex_)> l(conns_mutex_);

    duration delta = (std::chrono::duration_cast<delta_duration>(tick_duration_) * nb_ticks).count();

    // If a player got disconnected, we remove the corresponding connection and entity objects
    std::list<id_t> ids_to_remove;
    for (auto const& p : conns_)
        if (p.second->is_closed())
            ids_to_remove.push_back(p.first);

    for (auto id : ids_to_remove)
    {
        assert(entities_.at(id)->type() == "player");
        LOG("SERVER LOOP", "Removing id " << id << " from connections and entities");

        if (conns_.erase(id) == 0)
            LOG("SERVER LOOP", "ERROR: No id in connections");
        if (entities_.erase(id) == 0)
            LOG("SERVER LOOP", "ERROR: No id in entities");
    }

    // Check network state
    assert(acceptor_.is_open());
    assert(!io_context_.stopped());

    // We build the remove message
    std::shared_ptr<std::string const> remove_msg;
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
        remove_msg = std::make_shared<std::string const>(std::move(ss.str()));
    }

    // Apply all pending patches of all connections
    for (auto &c : conns_)
    {
        auto id = c.second->player_entity()->id();
        assert(entities_.count(id) == 1);
        ws_conn::patch p;
        while (c.second->pop_patch(p))
        {
            try {
                if (p.what == "speed")
                    entities_[id]->set_speed(any_cast<float>(p.value));
                else if (p.what == "vel")
                    entities_[id]->set_dir(any_cast<vector>(p.value));
                else if (p.what == "targetPos")
                    entities_[id]->set_dir(any_cast<vector>(p.value) - entities_[id]->pos());
                else
                    LOG("SERVER LOOP", "ERROR during patches application: unknown value: " << p.what);
            }
            catch (const bad_any_cast& e) {
                LOG("SERVER LOOP", "ERROR during patches application: " << e.what() << ". Conn: " << c.second->addr_str << ", entity: " << id << ", value name: " << p.what);
            }
        }
    }

    // Update all entities with delta
    for (auto &p : entities_)
    {
        env env(entities_);
        // Remove the entity from its own env so it doesn't see itself
        assert(env.others().count(p.second->id()) == 1);
        assert(env.others().at(p.second->id()).use_count() > 1);
        env.others().erase(p.second->id());
        p.second->update(delta, env);
    }

    // Serialize all entities
    std::shared_ptr<std::string const> jsonentities = std::make_shared<std::string const>(std::move(entities_to_json(entities_)));

    // We broadcast all changes to all players
    for (auto &c : server::conns_)
    {
        if (c.second->current_state() != ws_conn::reading
            && c.second->current_state() != ws_conn::writing)
            continue;

        if (remove_msg)
            c.second->write(remove_msg);

        c.second->write(jsonentities);
    }
}

void server::on_accept(const boost::system::error_code& ec) noexcept
{
    std::lock_guard<decltype(conns_mutex_)> l(conns_mutex_);

    if (ec)
    {
        if (ec.value() == 995)
            LOG("SERVER ACCEPT", "INFO: operation aborted");
        else
            LOG("SERVER ACCEPT", "ERROR " << ec.value() << ": " << ec.message());

        if (!*stop_)
            do_accept();
        return;
    }

    std::shared_ptr<entity> e;
    std::shared_ptr<ws_conn> new_connection;

    try {
        e = add_entity({ 0.f, 0.f }, { 0.f, 0.f }, 1.f, 1.f, "player");

        new_connection = std::make_shared<ws_conn>(std::ref(new_client_socket_), e);

        assert(conns_.insert({ e->id(), new_connection }).second);

        new_connection->start();

        LOG("SERVER ACCEPT", "New connection from: " << new_connection->addr_str);
    }
    catch (...) {
        LOG("SERVER LOOP", "ERROR while creating new player entity, new connection or while starting it");

        if (e)
            conns_.erase(e->id());
        if (new_connection)
            conns_.erase(e->id());
    }
    do_accept();
}

void server::do_accept()
{
    acceptor_.async_accept(new_client_socket_, std::bind(&server::on_accept, shared_from_this(), std::placeholders::_1));
}

// totest: Args... and std::forward ?
std::shared_ptr<entity> server::add_entity(vector const& pos, vector const& dir, real speed, real max_speed, std::string const& type, behaviors && behaviors)
{
    std::shared_ptr<entity> e = std::make_shared<entity>(pos, dir, speed, max_speed, type, std::move(behaviors));
    auto r = entities_.insert(std::make_pair(e->id(), e));
    assert(r.second);
    return e;
}
