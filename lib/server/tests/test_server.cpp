#include <future>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <gtest/gtest.h>

#include <webgame/npc.hpp>
#include <webgame/persistence.hpp>
#include <webgame/protocol.hpp>
#include <webgame/redis_persistence.hpp>
#include <webgame/server.hpp>
#include <webgame/utils.hpp>
#include <webgame/vector.hpp>

#include "tests.hpp"

std::mt19937 g(0);
std::uniform_real_distribution<> dis(-1.0, 1.0);
char const consonants[] = "bcdfghjklmnpqrstvwxz";
char const vowels[] = "aeiouy";
std::uniform_int_distribution<unsigned int> consonants_rd(0, sizeof(consonants) / sizeof(*consonants) - 2);
std::uniform_int_distribution<unsigned int> vowels_rd(0, sizeof(vowels) / sizeof(*vowels) - 2);
unsigned int const nb_syllabus = 5;
std::string gen_name()
{
    std::string name;
    name.reserve(nb_syllabus * 2 + 1);
    for (int i = 0; i < nb_syllabus; ++i)
    {
        name.push_back(consonants[consonants_rd(g)]);
        name.push_back(vowels[vowels_rd(g)]);
    }
    return name;
}

using dur = std::chrono::duration<float>;

// Server orders
struct state_game_order
{
    double tick_duration;
};

struct state_player_order
{
    webgame::vector pos;
    webgame::vector dir;
    std::string type;
    double speed;

    bool operator==(state_player_order const& other) const
    {
        return pos == other.pos
            && dir == other.dir
            && type == other.type
            && speed == other.speed;
    }

    bool operator!=(state_player_order const& other) const
    {
        return !(*this == other);
    }
};

struct state_entities_order
{
    struct entity_state
    {
        size_t id;
        std::string type;
        webgame::vector pos;
        webgame::vector dir;
    };
    std::vector<entity_state> data;
};

// Client orders
struct authentication_order
{
    std::string player_name;
    authentication_order(std::string const&pn)
        : player_name(pn)
    {}
};

struct action_change_speed_order
{
    double speed;
    action_change_speed_order(double s)
        : speed(s)
    {}
};

struct action_change_dir_order
{
    webgame::vector dir;
    action_change_dir_order(webgame::vector const&v)
        : dir(v)
    {}
};

struct action_move_to_order
{
    webgame::vector target_pos;
    action_move_to_order(webgame::vector const&v)
        : target_pos(v)
    {}
};

//boost::property_tree::ptree get_ptree(authentication_order const& order)
//{
//    boost::property_tree::ptree ptree;
//    ptree.put<std::string>("order", "authentication", my_id_translator<std::string>());
//    ptree.put<std::string>("player_name", order.player_name, my_id_translator<std::string>());
//    /*ptree.put("order", "authentication");
//    ptree.put("player_name", order.player_name);*/
//    return ptree;
//}
//
//boost::property_tree::ptree get_ptree(action_move_to_order const& order)
//{
//    boost::property_tree::ptree ptree;
//    ptree.put<std::string>("order", "action", my_id_translator<std::string>());
//    ptree.put<std::string>("suborder", "move_to", my_id_translator<std::string>());
//    /*ptree.put("order", "action");
//    ptree.put("suborder", "move_to");*/
//    ptree.add_child("target_pos", get_ptree(order.target_pos));
//    return ptree;
//}

template<class T>
T get_order(boost::property_tree::ptree const& ptree)
{}

template<>
state_game_order get_order(boost::property_tree::ptree const& ptree)
{
    assert(ptree.get<std::string>("order") == "state");
    assert(ptree.get<std::string>("suborder") == "game");

    state_game_order o;
    o.tick_duration = ptree.get<double>("tick_duration");
    return o;
}

template<>
state_player_order get_order(boost::property_tree::ptree const& ptree)
{
    assert(ptree.get<std::string>("order") == "state");
    assert(ptree.get<std::string>("suborder") == "player");

    state_player_order o;
    o.dir = webgame::vector({ ptree.get<double>("dir.x"), ptree.get<double>("dir.y") });
    o.pos = webgame::vector({ ptree.get<double>("pos.x"), ptree.get<double>("pos.y") });
    o.type = ptree.get<std::string>("type");
    o.speed = ptree.get<double>("speed");
    return o;
}

struct test_bot
{
public:
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> socket;
    boost::beast::multi_buffer buffer;
    std::string name;
    std::string last_read;

    test_bot(boost::asio::io_context &io_context, std::string const& n = "none")
        : socket(io_context)
        , name(n)
    {
        connect(io_context);
    }

    void connect(boost::asio::io_context &io_context)
    {
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::resolver::results_type results = resolver.resolve("localhost", "2000");
        boost::asio::connect(socket.next_layer(), results.begin(), results.end());
        socket.handshake("localhost", "/");
    }

    boost::beast::error_code read_ec()
    {
        boost::beast::error_code ec;
        socket.read(buffer, ec);
        last_read = boost::beast::buffers_to_string(buffer.data());
        buffer.consume(buffer.size());
        return ec;
    }

    boost::beast::error_code read_until_error()
    {
        boost::beast::error_code ec;
        while (!(ec = read_ec()));
        return ec;
    }

    void read()
    {
        socket.read(buffer);
        last_read = boost::beast::buffers_to_string(buffer.data());
        buffer.consume(buffer.size());
        //std::cout << last_read << std::endl;
    }

    boost::property_tree::ptree read_ptree()
    {
        read();
        boost::property_tree::ptree ptree;
        std::istringstream iss(std::move(last_read));
        boost::property_tree::read_json(iss, ptree);
        return ptree;
    }

    boost::property_tree::ptree read_ptree_until_order(std::string const& order, std::string const& suborder, unsigned int n = 1)
    {
        boost::property_tree::ptree ptree;
        for (unsigned int i = 0; i < n; ++i)
        {
            do
            {
                read();
                std::istringstream iss(std::move(last_read));
                boost::property_tree::read_json(iss, ptree);
            } while (ptree.get<std::string>("order") != order || ptree.get<std::string>("suborder") != suborder);
        }
        return ptree;
    }

    /*void write(boost::property_tree::ptree const& ptree)
    {
        socket.write(boost::asio::buffer(get_json(ptree)));
    }*/

    void send_order(authentication_order const& order)
    {
        socket.write(boost::asio::buffer("{\"order\":\"authentication\""
            ",\"player_name\":\"" + order.player_name + "\"}"));
    }

    void send_order(action_change_speed_order const& order)
    {
        socket.write(boost::asio::buffer("{\"order\":\"action\""
            ",\"suborder\":\"change_speed\""
            ",\"speed\":" + std::to_string(order.speed) + "}"));
    }

    void send_order(action_change_dir_order const& order)
    {
        socket.write(boost::asio::buffer("{\"order\":\"action\""
            ",\"suborder\":\"change_dir\""
            ",\"dir\":" + order.dir.save().dump() + "}"));
    }

    void send_order(action_move_to_order const& order)
    {
        socket.write(boost::asio::buffer("{\"order\":\"action\""
            ",\"suborder\":\"move_to\""
            ",\"target_pos\":" + order.target_pos.save().dump() + "}"));
    }


    std::pair<state_game_order, state_player_order> authenticate()
    {
        //write(get_ptree(authentication_order(name)));
        send_order(authentication_order(name));
        auto game_state = get_order<state_game_order>(read_ptree());
        auto player_state = get_order<state_player_order>(read_ptree());
        return std::make_pair(game_state, player_state);
    }

    void change_speed(double new_speed)
    {
        send_order(action_change_speed_order(new_speed));
    }

    void change_dir(webgame::vector const& new_dir)
    {
        send_order(action_change_dir_order(new_dir));
    }

    void move_to(webgame::vector const& to)
    {
        //write(get_ptree(action_move_to_order(to)));
        send_order(action_move_to_order(to));
    }
};

#define RESETDB \
do {\
    std::shared_ptr<webgame::persistence> redis = std::make_shared<webgame::redis_persistence>(io_context, "localhost", 6379, 1);\
    ASSERT_NO_THROW(ASSERT_TRUE(redis->start());\
    redis->remove_all());\
} while (0)

TEST(server, run)
{
    boost::asio::io_context io_context;
    RESETDB;

    std::shared_ptr<webgame::server> wg;
    ASSERT_NO_THROW(wg = std::make_shared<webgame::server>(io_context, 2000, std::make_shared<webgame::redis_persistence>(io_context, "localhost", 6379, 1)));
    std::future<void> fut = std::async(std::launch::async, [wg, &io_context] {
        ASSERT_NO_THROW(wg->start());
        ASSERT_NO_THROW(io_context.run_for(std::chrono::duration_cast<std::chrono::steady_clock::duration>(dur(2))));
    });
    // Impossible to predict these durations due to unknown threads scheduling
    // ASSERT_EQ(std::future_status::timeout, fut.wait_for(dur(2)));
    // ASSERT_EQ(std::future_status::ready, fut.wait_for(dur(2)));
    ASSERT_FALSE(io_context.stopped());
}

#define RUN \
boost::asio::io_context io_context;\
RESETDB;\
std::shared_ptr<webgame::server> wg = std::make_shared<webgame::server>(io_context, 2000, std::make_shared<webgame::redis_persistence>(io_context, "localhost", 6379, 1));\
wg->start();\
std::future<void> run_fut = std::async(std::launch::async, [wg, &io_context] {\
    try {\
        io_context.run();\
    } catch (std::exception const& e){\
        std::cout << "Exception thrown in run async function: " << e.what() << std::endl;\
    }\
});\
webgame::on_scope stop_on_ret([]{}, [&io_context] {\
        std::this_thread::sleep_for(dur(3));\
        ASSERT_TRUE(io_context.stopped());\
        io_context.stop();\
});

TEST(server, shutdown_no_clients)
{
    RUN;

    ASSERT_NO_THROW(wg->shutdown());
}

TEST(server, shutdown_one_client_ack)
{
    RUN;

    test_bot bot1(io_context);

    ASSERT_NO_THROW(wg->shutdown());
    // Impossible to predict this durations due to unknown threads scheduling
    // ASSERT_EQ(std::future_status::timeout, run_fut.wait_for(dur(10)));
    EXPECT_TRUE(bot1.socket.is_open());
    ASSERT_EQ(boost::beast::websocket::error::closed, bot1.read_until_error());
    EXPECT_FALSE(bot1.socket.is_open());
    ASSERT_EQ(std::future_status::ready, run_fut.wait_for(dur(15)));
}

#define PREPARE \
RUN;\
std::list<test_bot> bots;\
webgame::on_scope os([] {}, [wg, &bots, &run_fut] {\
    wg->shutdown();\
    for (auto &bot : bots)\
        EXPECT_EQ(boost::beast::websocket::error::closed, bot.read_until_error());\
    run_fut.wait();\
})

#define ADD_BOT(name) do { bots.emplace_back(io_context, name); } while (0)
#define BOT(name) ADD_BOT(#name); test_bot &name = bots.back()

TEST(server, authenticate_new_player)
{
    PREPARE;

    BOT(bot1);

    ASSERT_NO_THROW(bot1.send_order(authentication_order(bot1.name)));

    boost::property_tree::ptree ptree;

    ASSERT_NO_THROW(ptree = bot1.read_ptree());

    ASSERT_TRUE(ptree.find("order") != ptree.not_found());
    ASSERT_EQ("state", ptree.get<std::string>("order"));
    ASSERT_TRUE(ptree.find("suborder") != ptree.not_found());
    ASSERT_EQ("game", ptree.get<std::string>("suborder"));
    ASSERT_TRUE(ptree.find("tick_duration") != ptree.not_found());
    ASSERT_GT(ptree.get<double>("tick_duration"), 0.01);


    ASSERT_NO_THROW(ptree = bot1.read_ptree());

    ASSERT_TRUE(ptree.find("order") != ptree.not_found());

    ASSERT_EQ("state", ptree.get<std::string>("order"));

    ASSERT_TRUE(ptree.find("suborder") != ptree.not_found());
    ASSERT_EQ("player", ptree.get<std::string>("suborder"));

    ASSERT_TRUE(ptree.find("dir") != ptree.not_found());
    ASSERT_TRUE(ptree.find("dir")->second.find("x") != ptree.not_found());
    ASSERT_EQ(0, ptree.get<double>("dir.x"));
    ASSERT_TRUE(ptree.find("dir")->second.find("y") != ptree.not_found());
    ASSERT_EQ(-1, ptree.get<double>("dir.y"));

    ASSERT_TRUE(ptree.find("pos") != ptree.not_found());
    ASSERT_TRUE(ptree.find("pos")->second.find("x") != ptree.not_found());
    ASSERT_EQ(0, ptree.get<double>("pos.x"));
    ASSERT_TRUE(ptree.find("pos")->second.find("y") != ptree.not_found());
    ASSERT_EQ(0, ptree.get<double>("pos.y"));

    ASSERT_TRUE(ptree.find("type") != ptree.not_found());
    ASSERT_EQ("player", ptree.get<std::string>("type"));

    ASSERT_TRUE(ptree.find("speed") != ptree.not_found());
    ASSERT_EQ(0, ptree.get<double>("speed"));

    ASSERT_TRUE(ptree.find("id") != ptree.not_found());

    ASSERT_TRUE(ptree.find("max_speed") == ptree.not_found());
}

TEST(server, player_move)
{
    PREPARE;

    BOT(bot1);
    std::pair<state_game_order, state_player_order> init_infos = bot1.authenticate();

    double tick_duration = init_infos.first.tick_duration;

    double speed = 0.25;
    webgame::vector init_pos = init_infos.second.pos;
    webgame::vector move({ 1, 1 });

    bot1.change_speed(speed);
    bot1.move_to(init_pos + move);

    webgame::vector prev_pos;

    auto fut = std::async(std::launch::async, [&init_pos, &bot1, &move, &prev_pos] {
        prev_pos = get_order<state_player_order>(bot1.read_ptree()).pos;
        do
        {
            webgame::vector current_pos = get_order<state_player_order>(bot1.read_ptree()).pos;
            if ((current_pos - init_pos)[0] < (prev_pos - init_pos)[0]
                || (current_pos - init_pos)[1] < (prev_pos - init_pos)[1])
                return false;
            TEST_LOG(current_pos << ", " << webgame::vector(current_pos - init_pos).norm());
            prev_pos = current_pos;
        } while (prev_pos[0] < (init_pos + move)[0] || prev_pos[1] < (init_pos + move)[1]);
        return true;
    });

    // double move_dur = move.norm() / speed;
    // TEST_LOG("real move duration: " << move_dur);
    // if (std::floor(move_dur / tick_duration) != move_dur)
    //     move_dur = std::floor(move_dur / tick_duration) * tick_duration + tick_duration;
    // TEST_LOG("tick duration: " << tick_duration);
    // TEST_LOG("move duration ceiled: " << move_dur);
    // ASSERT_EQ(std::future_status::timeout, fut.wait_for(dur(move_dur * 0.95)));
    // ASSERT_EQ(std::future_status::ready, fut.wait_for(dur(move_dur * 0.05)));
    ASSERT_TRUE(fut.get());
    ASSERT_EQ(webgame::vector(init_pos + move), prev_pos);
    ASSERT_EQ(webgame::vector(init_pos + move), get_order<state_player_order>(bot1.read_ptree()).pos);


    for (int i = 0; i < 4; ++i)
        bot1.read();
    bot1.change_dir({ -1, 0 });
    bot1.change_speed(1);

    for (double duration = 0; duration < 2; duration += tick_duration)
        bot1.read();
    webgame::vector current_pos = get_order<state_player_order>(bot1.read_ptree()).pos;
    ASSERT_GE(std::abs(webgame::vector(current_pos - prev_pos).x()), 2);
    ASSERT_LE(std::abs(webgame::vector(current_pos - prev_pos).x()), 2.5);
    ASSERT_EQ(1, current_pos.y());
}

TEST(server, player_reconnect)
{
    PREPARE;

    webgame::vector init_pos;

    BOT(bot1);
    state_player_order init_state = bot1.authenticate().second;
    init_pos = get_order<state_player_order>(bot1.read_ptree()).pos;
    bot1.change_speed(1);
    bot1.move_to(init_state.pos + webgame::vector({ 5, 0 }));
    std::this_thread::sleep_for(dur(1.5));

    bot1.socket.close(boost::beast::websocket::close_code::none);
    std::this_thread::sleep_for(dur(1));
    bot1.connect(io_context);

    bot1.authenticate();
    webgame::vector current_pos = get_order<state_player_order>(bot1.read_ptree()).pos;
    ASSERT_GE(std::abs(webgame::vector(current_pos - init_pos).x()), 1.5);
    ASSERT_LE(std::abs(webgame::vector(current_pos - init_pos).x()), 2);
    std::this_thread::sleep_for(dur(0.5));
    current_pos = get_order<state_player_order>(bot1.read_ptree()).pos;
    ASSERT_GE(std::abs(webgame::vector(current_pos - init_pos).x()), 2);
    ASSERT_LE(std::abs(webgame::vector(current_pos - init_pos).x()), 2.5);

    bot1.socket.close(boost::beast::websocket::close_code::none);
    std::this_thread::sleep_for(dur(1));
    bot1.connect(io_context);

    bot1.authenticate();
    current_pos = get_order<state_player_order>(bot1.read_ptree()).pos;
    ASSERT_GE(std::abs(webgame::vector(current_pos - init_pos).x()), 2);
    ASSERT_LE(std::abs(webgame::vector(current_pos - init_pos).x()), 2.5);
}

TEST(server, persistence)
{
    boost::asio::io_context io_context;
    RESETDB;
    std::shared_ptr<webgame::server> wg = std::make_shared<webgame::server>(io_context, 2000, std::make_shared<webgame::redis_persistence>(io_context, "localhost", 6379, 1));

    webgame::vector bot1_target_pos;
    webgame::vector bot2_target_pos;
    state_player_order bot1_state;
    state_player_order bot2_state;
    {
        std::future<void> run1_fut = std::async(std::launch::async, [wg, &io_context] {
            wg->start(std::chrono::milliseconds(250));
            ASSERT_NO_THROW(io_context.run());
        });
        webgame::on_scope stop_on_ret([] {}, [&io_context] {
            io_context.stop();
        });
        std::this_thread::sleep_for(dur(3));

        test_bot bot1(io_context, "bot1");
        test_bot bot2(io_context, "bot2");

        auto bot1_init_states = bot1.authenticate();
        double tick_duration = bot1_init_states.first.tick_duration;
        webgame::vector bot1_init_pos = bot1_init_states.second.pos;
        webgame::vector bot2_init_pos = bot2.authenticate().second.pos;

        bot1.change_speed(1);
        bot2.change_speed(1);

        bot1_target_pos = bot1_init_pos + webgame::vector({ -10, 0 });
        bot2_target_pos = bot2_init_pos + webgame::vector({ 10, 0 });
        bot1.move_to(bot1_target_pos);
        bot2.move_to(bot2_target_pos);

        unsigned int n = std::max(2U, static_cast<unsigned int>(std::ceil(1. / tick_duration)));
        bot1_state = get_order<state_player_order>(bot1.read_ptree_until_order("state", "player", n));
        bot2_state = get_order<state_player_order>(bot2.read_ptree_until_order("state", "player", n));

        ASSERT_LT(bot1_state.pos.x(), bot1_init_pos.x());
        ASSERT_EQ(bot1_state.pos.y(), bot1_init_pos.y());
        ASSERT_GT(bot2_state.pos.x(), bot2_init_pos.x());
        ASSERT_EQ(bot2_state.pos.y(), bot2_init_pos.y());

        bot1_state = get_order<state_player_order>(bot1.read_ptree_until_order("state", "player", n * 3));
        bot2_state = get_order<state_player_order>(bot2.read_ptree_until_order("state", "player", n * 3));

        TEST_LOG("CLOSING BOTS CONNECTIONS");
        bot1.socket.close(boost::beast::websocket::close_code::none);
        bot2.socket.close(boost::beast::websocket::close_code::none);

        wg->shutdown();
        ASSERT_EQ(std::future_status::ready, run1_fut.wait_for(dur(10)));
        TEST_LOG("SERVER STOPPED");
    }

    io_context.restart();

    {
        std::future<void> run2_fut = std::async(std::launch::async, [wg, &io_context] {
            ASSERT_NO_THROW(wg->start(std::chrono::milliseconds(424)));
            ASSERT_NO_THROW(io_context.run());
        });
        TEST_LOG("SERVER RESTARTED");
        webgame::on_scope stop_on_ret([] {}, [&io_context] {
            io_context.stop();
        });
        std::this_thread::sleep_for(dur(3));

        test_bot bot1(io_context, "bot1");
        test_bot bot2(io_context, "bot2");

        auto bot1_init_states = bot1.authenticate();
        double tick_duration = bot1_init_states.first.tick_duration;
        ASSERT_EQ(0.424, tick_duration);
        state_player_order bot1_new_state = bot1_init_states.second;
        state_player_order bot2_new_state = bot2.authenticate().second;

        ASSERT_EQ(bot1_state.pos, bot1_new_state.pos);
        ASSERT_EQ(bot2_state.pos, bot2_new_state.pos);

        ASSERT_EQ(bot1_state.dir, bot1_new_state.dir);
        ASSERT_EQ(bot2_state.dir, bot2_new_state.dir);

        ASSERT_EQ(bot1_state.type, bot1_new_state.type);
        ASSERT_EQ(bot2_state.type, bot2_new_state.type);

        ASSERT_EQ(bot1_state.speed, bot1_new_state.speed);
        ASSERT_EQ(bot2_state.speed, bot2_new_state.speed);

        bot1.read_ptree_until_order("state", "player");
        bot1_new_state = get_order<state_player_order>(bot1.read_ptree_until_order("state", "player"));
        bot2_new_state = get_order<state_player_order>(bot2.read_ptree_until_order("state", "player"));

        ASSERT_TRUE(almost_equal(bot1_state.pos.x() - 0.424 * 2, bot1_new_state.pos.x()));
        ASSERT_TRUE(almost_equal(bot2_state.pos.x() + 0.424, bot2_new_state.pos.x()));

        bot2_new_state = get_order<state_player_order>(bot2.read_ptree_until_order("state", "player"));

        unsigned int bot1_nb_ticks_to_target = static_cast<unsigned int>(std::ceil(webgame::vector(bot1_target_pos - bot1_new_state.pos).norm() / bot1_new_state.speed / tick_duration));
        unsigned int bot2_nb_ticks_to_target = static_cast<unsigned int>(std::ceil(webgame::vector(bot2_target_pos - bot2_new_state.pos).norm() / bot2_new_state.speed / tick_duration));

        bot1_new_state = get_order<state_player_order>(bot1.read_ptree_until_order("state", "player", bot1_nb_ticks_to_target));
        bot2_new_state = get_order<state_player_order>(bot2.read_ptree_until_order("state", "player", bot2_nb_ticks_to_target));

        ASSERT_EQ(bot1_target_pos, bot1_new_state.pos);
        ASSERT_EQ(bot2_target_pos, bot2_new_state.pos);

        bot1.socket.close(boost::beast::websocket::close_code::none);
        bot2.socket.close(boost::beast::websocket::close_code::none);

        // wg->shutdown();
        // ASSERT_EQ(std::future_status::ready, run2_fut.wait_for(dur(10)));
    }
}

//
//TEST(server, one_player_one_npe)
//{
//    boost::asio::io_context io_context;
//
//    {
//        std::shared_ptr<persistence> redis = std::make_shared<redis_persistence>(io_context, "localhost", 6379, 1);
//        redis->start();
//        redis->remove_all();
//        redis->async_save(entities({ std::make_shared<npc>("npe1", vector({ 0.5, -0.5 }),vector({ 1.f, -1 }), 0.2, 0.2) }), [] {});
//        io_context.run();
//        io_context.restart();
//    }
//
//    std::shared_ptr<server> wg;
//    ASSERT_NO_THROW(wg = std::make_shared<server>(io_context, 2000, std::make_shared<redis_persistence>(io_context, "localhost", 6379, 1)));
//    std::future<void> fut = std::async(std::launch::async, [wg, &io_context] {
//        ASSERT_NO_THROW(wg->start());
//        ASSERT_NO_THROW(io_context.run_for(std::chrono::duration_cast<std::chrono::steady_clock::duration>(dur(2))));
//    });
//    ASSERT_EQ(std::future_status::timeout, fut.wait_for(dur(2)));
//    ASSERT_EQ(std::future_status::ready, fut.wait_for(dur(2)));
//    ASSERT_FALSE(io_context.stopped());
//}

//TEST(server, siege)
//{
//    PREPARE;
//
//    while (size_t i = 0; i < 1000; ++i)
//        ADD_BOT("bot" + std::to_string(i));
//
//    // Same behavior as webgame-bots
//}

TEST(server, shutdown_one_client_timeout)
{
    RUN;

    test_bot bot1(io_context);

    ASSERT_NO_THROW(wg->shutdown());
    std::this_thread::sleep_for(dur(2));
    EXPECT_TRUE(bot1.socket.is_open());
    std::this_thread::sleep_for(dur(10));
    bot1.read_until_error();
    EXPECT_FALSE(bot1.socket.is_open());
}
