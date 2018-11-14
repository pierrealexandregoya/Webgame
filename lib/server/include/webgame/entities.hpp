#pragma once

#include <initializer_list>
#include <map>
#include <memory>

#include "common.hpp"
#include "entity.hpp"

namespace webgame {

template<class EntityType>
class entity_container : public std::map<id_t, std::shared_ptr<EntityType> const>
{
public:
    entity_container() = default;
    entity_container(std::initializer_list<std::shared_ptr<EntityType>> const& ilist);

public:
    std::shared_ptr<EntityType> const& add(std::shared_ptr<EntityType> const& ent_ptr);

    template<class SubType>
    operator entity_container<SubType>();
};

class entity;
using entities = entity_container<entity>;

} // namespace webgame

#include "entities.hxx"
