#include "entities.hpp"

#include "entity.hpp"

std::shared_ptr<entity> const& entities::add(vector const& pos, vector const& dir, real speed, real max_speed, std::string const& type, behaviors_t && behaviors)
{
    return add(entity(pos, dir, speed, max_speed, type, std::move(behaviors)));
}

std::shared_ptr<entity> const& entities::add(entity && ent)
{
    std::shared_ptr<entity> ent_ptr = std::make_shared<entity>(std::move(ent));
    return add(const_cast<std::shared_ptr<entity> const&>(ent_ptr));
}

std::shared_ptr<entity> const& entities::add(std::shared_ptr<entity> const& ent_ptr)
{
    auto ret = this->insert(std::make_pair(ent_ptr->id(), ent_ptr));
    assert(ret.second);
    return ret.first->second;
}
