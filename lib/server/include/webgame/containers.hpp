#pragma once

#include <map>
#include <memory>

#include "common.hpp"

class behavior;
typedef std::multimap<int, std::shared_ptr<behavior> /*const*/> behaviors_t;

class ws_conn;
typedef std::map<id_t, std::shared_ptr<ws_conn> const> connections;
