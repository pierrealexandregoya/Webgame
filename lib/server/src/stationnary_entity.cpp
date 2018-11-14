#include "stationnary_entity.hpp"

#include "save_load.hpp"

namespace webgame {

stationnary_entity::stationnary_entity(std::string const& type, vector const& pos)
    : located_entity(type, pos)
{}

bool stationnary_entity::update(double d, env & env)
{
    return false;
}

nlohmann::json stationnary_entity::save() const
{
    return {
        {"located_entity", located_entity::save()},
        {"type", "stationnary_entity"}
    };
}

void stationnary_entity::load(nlohmann::json const& j)
{
    if (!j.is_object() || !j.count("located_entity"))
        throw std::runtime_error("stationnary_entity: invalid JSON");

    located_entity::load(j["located_entity"]);
}

WEBGAME_REGISTER(entity, stationnary_entity);

} // namespace webgame
