#include "redis_persistence.hpp"

#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "entities.hpp"
#include "log.hpp"
#include "npc.hpp"
#include "player.hpp"
#include "redis_helper.hpp"
#include "save_load.hpp"

namespace asio = boost::asio;

namespace webgame {

redis_persistence::redis_persistence(boost::asio::io_context &io_context, std::string const& host, unsigned short port, unsigned int index)
    : socket_(io_context)
    , host_(host)
    , port_(port)
    , index_(index)
    , io_context_(io_context)
{}

bool redis_persistence::start()
{
    try {
        asio::ip::tcp::resolver resolver(io_context_);

        asio::ip::basic_resolver_results<asio::ip::tcp> resolve_results = resolver.resolve(host_, std::to_string(port_));

        asio::connect(socket_, resolve_results.cbegin(), resolve_results.cend());

        helper_ = std::make_shared<redis_helper>(socket_);

        helper_->select(index_);

        return true;
    }
    catch (std::exception const& e) {
        helper_.reset();
        WEBGAME_LOG("REDIS", "Could not connect to redis: " << e.what());
        return false;
    }
}

void redis_persistence::stop()
{
    helper_.reset();
}

void redis_persistence::async_save(entities const& ents, std::function<save_handler> &&handler)
{
    std::vector<std::pair<std::string, std::string>> keys_values;

    keys_values.reserve(ents.size());
    for (auto const& e : ents)
    {
        std::string key;
        if (e.second->type().find("_player") != std::string::npos)
            key = "player";
        else
            key = "npe";
        key += ":" + std::to_string(e.first);

        std::string value = e.second->save().dump();

        keys_values.emplace_back(std::make_pair(std::move(key), std::move(value)));
    }

    auto this_p = shared_from_this();
    auto handler_p = std::make_shared<std::function<save_handler>>(std::move(handler));
    helper_->async_multi_set(keys_values, [this_p, handler_p] {
        (*handler_p)();
    });
}

entities redis_persistence::load_all_npes()
{
    WEBGAME_LOG("REDIS", "LOADING ALL NON PLAYABLE ENTITIES");

    std::vector<std::string> keys = helper_->keys("npe:*");

    if (keys.empty())
    {
        WEBGAME_LOG("REDIS", "NOTHING TO LOAD");
        return entities();
    }

    std::vector<std::string> values = helper_->multi_get(keys);

    entities ents;
    for (std::string const& value : values)
        ents.add(load_entity(nlohmann::json::parse(value)));
    return ents;
}

//void redis_persistence::async_load_player(std::string const& name, std::function<load_player_handler> &&handler)
//{
//    WEBGAME_LOG("REDIS", "LOAD PLAYER " << name);
//
//    auto this_p = shared_from_this();
//    auto name_p = std::make_shared<std::string>(name);
//    auto handler_p = std::make_shared<std::function<load_player_handler>>(std::move(handler));
//
//    helper_->async_get("playername:" + name, [this_p, name_p, handler_p](bool success, std::string && value) {
//        // Player does not exist yet
//        if (!success)
//        {
//            WEBGAME_LOG("REDIS", "PLAYER DOES NOT EXIST, CREATING IT");
//            // We create a default entity
//            auto new_ent = std::make_shared<player>();
//
//            WEBGAME_LOG("REDIS", "STORING IT IN player:<id> table");
//            // We serialize it and store it in player:<id> table
//            this_p->helper_->async_set("player:" + std::to_string(new_ent->id()), new_ent->save().dump(), [this_p, name_p, handler_p, new_ent]() {
//
//                WEBGAME_LOG("REDIS", "STORING ITS ID IN playername:<name> table");
//                // We set its id in playername:<name> table
//                this_p->helper_->async_set("playername:" + *name_p, std::to_string(new_ent->id()), [this_p, handler_p, new_ent]() {
//
//                    WEBGAME_LOG("REDIS", "LOAD PLAYER DONE, CALLING HANDLER");
//                    (*handler_p)(new_ent);
//                });
//
//            });
//        }
//        // Player exists, we get its serialization and create an entity
//        else
//        {
//            WEBGAME_LOG("REDIS", "PLAYER EXISTS, LOADING IT");
//            this_p->helper_->async_get("player:" + value, [this_p, name_p, handler_p](bool success, std::string && value) {
//                if (!success)
//                    throw std::runtime_error("redis_persistence: player's id found but the corresponding serialized entity does not exist");
//
//                WEBGAME_LOG("REDIS", "LOAD PLAYER DONE, CALLING HANDLER");
//                (*handler_p)(std::dynamic_pointer_cast<player>(load_entity(nlohmann::json::parse(value))));
//            });
//        }
//    });
//}

void redis_persistence::async_check_player(std::string const& name, std::function<check_player_handler> &&handler)
{
    auto this_p = shared_from_this();
    auto handler_p = std::make_shared<std::function<check_player_handler>>(std::move(handler));

    helper_->async_get("playername:" + name, [this_p, handler_p](bool success, std::string && value) {
        if (success)
            (*handler_p)(true, std::stoul(value));
        else
            (*handler_p)(false, 0);
    });
}

void redis_persistence::async_get_player(id_t id, std::function<get_player_handler> &&handler)
{
    auto this_p = shared_from_this();
    auto handler_p = std::make_shared<std::function<get_player_handler>>(std::move(handler));

    this_p->helper_->async_get("player:" + std::to_string(id), [this_p, handler_p](bool success, std::string && value) {

        if (success)
            (*handler_p)(true, std::dynamic_pointer_cast<player>(load_entity(nlohmann::json::parse(value))));
        else
            (*handler_p)(false, nullptr);
    });
}

void redis_persistence::async_add_player(std::string const& name, std::shared_ptr<player> const& player, std::function<add_player_handler> &&handler)
{
    auto this_p = shared_from_this();
    auto handler_p = std::make_shared<std::function<add_player_handler>>(std::move(handler));

    this_p->helper_->async_set("player:" + std::to_string(player->id()), player->save().dump(), [this_p, name, player, handler_p]() {

        this_p->helper_->async_set("playername:" + name, std::to_string(player->id()), [this_p, handler_p]() {
            (*handler_p)();
        });

    });
}

void redis_persistence::remove_all()
{
    helper_->flushdb();
}

} // namespace webgame
