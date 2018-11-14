#include <boost/asio/ip/tcp.hpp>

#include <bredis/Connection.hpp>
#include <bredis/MarkerHelpers.hpp>

#include <webgame/behavior.hpp>
#include <webgame/entities.hpp>
#include <webgame/entity.hpp>
#include <webgame/log.hpp>
#include <webgame/npc.hpp>
#include <webgame/redis_persistence.hpp>
#include <webgame/stationnary_entity.hpp>

namespace asio = boost::asio;

using buffer_t = boost::asio::streambuf;
using it_t = typename bredis::to_iterator<buffer_t>::iterator_t;
using policy_t = bredis::parsing_policy::keep_result;
using result_t = bredis::parse_result_mapper_t<it_t, policy_t>;

int main()
{
    webgame::entities ents;
    // First ally NPC
    auto &ally = ents.add(std::make_shared<webgame::npc>("npc_ally_1", webgame::vector({ 0.5, 0.5 }), webgame::vector({ 0, 0 }), 0.2, 0.2, webgame::npc::behaviors({
        { 0, std::make_shared<webgame::arealimit>(webgame::arealimit::square, 0.5, webgame::vector({ 0.5, 0.5 })) } ,
        { 10, std::make_shared<webgame::walkaround>() },
        })));
    WEBGAME_LOG("INFO", "TEST ALLY ID IS " << ally->id());

    // First enemy NPC
    auto &enemy = ents.add(std::make_shared<webgame::npc>("npc_enemy_1", webgame::vector({ -0.5, -0.5 }), webgame::vector({ 0, 0 }), 0.4, 0.4, webgame::npc::behaviors({
        { 0, std::make_shared<webgame::arealimit>(webgame::arealimit::square, 0.5f, webgame::vector({ -0.5, -0.5 })) } ,
        { 10, std::make_shared<webgame::attack_on_sight>(0.7) },
        { 20, std::make_shared<webgame::stop>() },
        //{ 1.f, std::make_shared<patrol>({,}, {,}, ...) },
        })));
    WEBGAME_LOG("INFO", "TEST ENEMY ID IS " << enemy->id());

    // 100 static objects centered at 0,0
    const int s = 10;
    for (int i = 0; i < s; ++i)
        for (int j = 0; j < s; ++j)
            ents.add(std::make_shared<webgame::stationnary_entity>("object1", webgame::vector({ i - s / 2. , j - s / 2. })));



    asio::io_context io_context;

    std::shared_ptr<webgame::persistence> redis = std::make_shared<webgame::redis_persistence>(io_context, "localhost");

    WEBGAME_LOG("RESET", "STARTING");
    if (!redis->start())
    {
        WEBGAME_LOG("RESET", "FAILED TO START");
        return 1;
    }

    WEBGAME_LOG("RESET", "REMOVING ALL");
    redis->remove_all();

    WEBGAME_LOG("RESET", "SAVING " << ents.size() << " TEST ENTITIES");
    redis->async_save(ents, [] {});
    io_context.run();
}
