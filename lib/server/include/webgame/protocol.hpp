#pragma once

#include <string>

#include "containers.hpp"

class entities;
class entity;

// Protocole helpers
extern std::string json_state_entities(entities const& entities);
extern std::string json_state_player(std::shared_ptr<entity const> e);
