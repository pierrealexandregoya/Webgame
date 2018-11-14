#pragma once

#include <memory>
#include <list>

namespace webgame {

class player_conn;
typedef std::list<std::shared_ptr<player_conn>> connections;

} // namespace webgame
