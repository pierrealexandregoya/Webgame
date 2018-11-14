#pragma once

#include <memory>
#include <string>

#include "config.hpp"
#include "entities.hpp"

namespace webgame {

class player;

WEBGAME_API extern std::string json_state_entities(entities const& entities);
WEBGAME_API extern std::string json_state_player(std::shared_ptr<player const> e);

} // namespace webgame
