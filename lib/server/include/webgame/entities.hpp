#pragma once

#include <map>
#include <memory>

#include "common.hpp"
#include "entity.hpp"

class entity;
class entities : public std::map<id_t, std::shared_ptr<entity> const>
{
public:
    std::shared_ptr<entity> const& add(vector const& pos, vector const& dir, real speed, real max_speed, std::string const& type, behaviors_t && behaviors=behaviors_t());
    std::shared_ptr<entity> const& add(entity && ent);
    std::shared_ptr<entity> const& add(std::shared_ptr<entity> const& ent_ptr);
};
