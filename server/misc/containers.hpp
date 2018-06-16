#pragma once

#include <map>
#include <memory>
class behavior;
typedef std::map<float, std::shared_ptr<behavior> const> behaviors;
class entity;
typedef std::map<unsigned int, std::shared_ptr<entity> const> entities;
class ws_conn;
typedef std::map<unsigned int, std::shared_ptr<ws_conn> const> connections;
