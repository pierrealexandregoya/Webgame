#include <boost/asio/ip/tcp.hpp>

#include <bredis/Connection.hpp>
#include <bredis/MarkerHelpers.hpp>

#include "entities.hpp"
#include "redis_conn.hpp"
#include "utils.hpp"

namespace asio = boost::asio;

using buffer_t = boost::asio::streambuf;
using it_t = typename bredis::to_iterator<buffer_t>::iterator_t;
using policy_t = bredis::parsing_policy::keep_result;
using result_t = bredis::parse_result_mapper_t<it_t, policy_t>;

int main()
{
    entities ents;
    // First ally NPC
    ents.add({ 0.5f, 0.5f }, { 0.f, 0.f }, 0.2f, 0.2f, "npc_ally_1", behaviors_t({
        { 0, std::make_shared<arealimit>(arealimit::Square, 0.5f, vector({ 0.5f, 0.5f })) } ,
        { 10, std::make_shared<walkaround>() },
        }));

    // First enemy NPC
    ents.add({ -0.5f, -0.5f }, { 0.f, 0.f }, 0.4f, 0.4f, "npc_enemy_1", behaviors_t({
        { 0, std::make_shared<arealimit>(arealimit::Square, 0.5f, vector({ -0.5f, -0.5f })) } ,
        { 10, std::make_shared<attack_on_sight>(0.7f) },
        { 20, std::make_shared<stop>() },
        //{ 1.f, std::make_shared<patrol>({,}, {,}, ...) },
        }));

    // 100 static objects centered at 0,0
    const int s = 10;
    for (int i = 0; i < s; ++i)
        for (int j = 0; j < s; ++j)
            ents.add({ i - s / 2.f , j - s / 2.f }, { 0.f, 0.f }, 0.f, 0.f, "object1");



    asio::io_context io_context;

    LOG("REDIS", "CONNECTING");
    asio::ip::tcp::resolver resolver(io_context);
    auto res = resolver.resolve("localhost", "6379");
    asio::ip::tcp::socket socket(io_context);
    asio::connect(socket, res.cbegin(), res.cend());
    LOG("REDIS", "CONNECTED");

    redis_conn redis(std::move(socket));

    redis.remove_all();

    redis.save(ents);
}
