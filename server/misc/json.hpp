#pragma once

#include <string>

#include <boost/property_tree/ptree.hpp>

#include "containers.hpp"

class entity;
class vector;

extern boost::property_tree::ptree  vec_to_ptree(vector const& v);
extern boost::property_tree::ptree  entity_to_ptree(std::shared_ptr<entity const> e);
extern std::string                  entities_to_json(entities const&);
