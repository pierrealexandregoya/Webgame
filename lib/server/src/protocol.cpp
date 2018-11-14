#include "protocol.hpp"

#include "entities.hpp"
#include "entity.hpp"
#include "player.hpp"

namespace webgame {

std::string json_state_entities(entities const& entities)
{
    nlohmann::json j = {
        {"order", "state"},
        {"suborder", "entities"},
        {"data", nlohmann::json::array()}
    };

    for (auto const& pair : entities)
    {
        nlohmann::json ent_j;
        pair.second->build_state_order(ent_j);
        j["data"].emplace_back(std::move(ent_j));
    }

    return j.dump();
}

std::string json_state_player(std::shared_ptr<player const> e)
{
    nlohmann::json j;
    j["order"] = "state";
    j["suborder"] = "player";
    e->build_state_order(j);

    return j.dump();
}

} // namespace webgame
