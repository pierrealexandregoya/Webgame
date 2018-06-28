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

void server::run()
{
    run_game();

    run_network();

    run_input_read();

    shutdown();
}

void server::shutdown()
{
    *stop_ = true;
    LOG("SHUTDOWN", "Stopping game loop thread");
    game_loop_status_.wait();

    LOG("SHUTDOWN", "Cancelling server accept");
    acceptor_.cancel();
    for (auto conn : conns_)
    {
        LOG("SHUTDOWN", "Closing connection " << conn.second->addr_str);
        conn.second->close();
    }

    LOG("SHUTDOWN", "Closing server socket");
    acceptor_.close();

    for (auto & t : network_threads_)
    {
        LOG("SHUTDOWN", "Joining network thread " << t.get_id());
        t.join();
    }
}

void server::run_game()
{
    wake_time_ = std::chrono::ceil<std::chrono::seconds>(steady_clock::now());
    assert(std::chrono::duration_cast<std::chrono::nanoseconds>(wake_time_.time_since_epoch()).count() % 1000000000 == 0);

    game_loop_status_ = std::async(std::launch::async, std::bind(&server::game_loop, this));
}

void server::run_network()
{
    asio::post(acceptor_.get_executor(), std::bind(&server::do_accept, shared_from_this()));

    network_threads_.reserve(threads_);
    for (auto i = 0; i < threads_; ++i)
        network_threads_.emplace_back([this, i] {
        LOG("NETWORK", "THREAD #" << i + 1 << " STARTED");
        try {
            io_context_.run();
        }
        catch (...) {
            assert(false);
        }
        assert(*stop_);
        LOG("NETWORK", "THREAD #" << i + 1 << " STOPPED");
    });
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

void server::run_input_read()
{
    std::string input;
    for (;;)
    {
        std::cin >> input;
        if (input == "info")
        {
            std::lock_guard<decltype(log_mutex)> lock(log_mutex);
            LOG("INFO", "Connections: " << conns_.size());
            LOG("INFO", "Entities: " << entities_.size());
            LOG("INFO", "Threads: " << network_threads_.size() << " asio thread"
                << (network_threads_.size() > 1 ? "s" : "") << " + user input thread + game loop thread");
        }
        else if (input == "help")
            show_help();
        else if (input == "data")
            data_log = !data_log;
        else if (input == "io")
            io_log = !io_log;
        else if (input == "exit")
            break;
        else
        {
            std::lock_guard<decltype(log_mutex)> lock(log_mutex);
            _MY_LOG("Unknown command: " << input << std::endl);
            show_help();
        }
    }
}

void server::game_loop()
{
    std::this_thread::sleep_until(wake_time_);

    LOG("GAME LOOP", "STARTED");

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
            LOG("GAME LOOP", "RETARD OF " << nb_ticks - 1 << " TICK" << (nb_ticks - 1 > 1 ? "S" : ""));
        std::this_thread::sleep_until(wake_time_);

        game_cycle(nb_ticks);
    }

    LOG("GAME LOOP", "STOPPED");
}

void server::game_cycle(unsigned int nb_ticks)
{
    std::lock_guard<decltype(server_mutex_)> l(server_mutex_);

    duration delta = (std::chrono::duration_cast<delta_duration>(tick_duration_) * nb_ticks).count();

    // If a player got disconnected, we remove the corresponding connection and entity objects
    std::list<id_t> ids_to_remove;
    for (auto const& p : conns_)
        if (p.second->is_closed())
            ids_to_remove.push_back(p.first);

    for (auto id : ids_to_remove)
    {
        assert(entities_.at(id)->type() == "player");
        LOG("GAME LOOP", "Removing id " << id << " from connections and entities");

        if (conns_.erase(id) == 0)
            LOG("GAME LOOP", "ERROR: No id in connections");
        if (entities_.erase(id) == 0)
            LOG("GAME LOOP", "ERROR: No id in entities");
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
                    LOG("GAME LOOP", "ERROR during patches application: unknown value: " << p.what);
            }
            catch (const bad_any_cast& e) {
                LOG("GAME LOOP", "ERROR during patches application: " << e.what() << ". Conn: " << c.second->addr_str << ", entity: " << id << ", value name: " << p.what);
            }
        }
    }

    // Update all entities with delta
    entities changed_entities;
    for (auto &p : entities_)
    {
        env env(entities_);
        // Remove the entity from its own env so it doesn't see itself
        env.others().erase(p.second->id());
        if (p.second->update(delta, env))
            changed_entities.insert(p);
    }

    // Serialize all entities with changes
    std::shared_ptr<std::string const> jsonentities;
    if (!changed_entities.empty())
        jsonentities = std::make_shared<std::string const>(json_state_entities(changed_entities));

    // We broadcast all changes to all players
    if (remove_msg)
        for (auto &c : server::conns_)
        {
            if (c.second->current_state() != ws_conn::reading
                && c.second->current_state() != ws_conn::writing)
                continue;

            c.second->write(remove_msg);
        }

    if (jsonentities)
        for (auto &c : server::conns_)
        {
            if (c.second->current_state() != ws_conn::reading
                && c.second->current_state() != ws_conn::writing)
                continue;

            c.second->write(jsonentities);
        }
}

void server::on_accept(const boost::system::error_code& ec) noexcept
{
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

    // MESSY
    std::shared_ptr<entity> new_ent;
    std::shared_ptr<ws_conn> new_conn;
    connections conns_copy;
    std::string json_entities;
    {
        std::lock_guard<decltype(server_mutex_)> l(server_mutex_);

        try {
            LOG("SERVER ACCEPT", "New connection from: " << new_client_socket_.remote_endpoint().address().to_string() + ":" + std::to_string(new_client_socket_.remote_endpoint().port()));

            new_ent = add_entity({ 0.f, 0.f }, { 0.f, 0.f }, 1.f, 1.f, "player");

            new_conn = std::make_shared<ws_conn>(std::ref(new_client_socket_), new_ent);

            assert(conns_.insert({ new_ent->id(), new_conn }).second);

            new_conn->start();

            conns_copy = conns_;
            json_entities = json_state_entities(entities_);
        }
        catch (...) {
            LOG("SERVER ACCEPT", "ERROR while creating new player entity, new connection or while starting it");

            if (new_ent)
            {
                if (new_conn)
                {
                    conns_.erase(new_ent->id());
                    new_conn.reset();
                }
                entities_.erase(new_ent->id());
                new_ent.reset();
            }

        }
    }

    if (new_ent && new_conn)
    {
        new_conn->write(std::make_shared<std::string const>(std::move(json_entities)));
        entities tmp = { {new_ent->id(), new_ent} };
        auto new_entity_json = std::make_shared<std::string const>(json_state_entities(tmp));
        for (auto &c : conns_copy)
            c.second->write(new_entity_json);
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
