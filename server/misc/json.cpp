#include "json.hpp"

#include <boost/property_tree/json_parser.hpp>

#include "common.hpp"
#include "behavior.hpp"
#include "entities.hpp"
#include "entity.hpp"

boost::property_tree::ptree get_ptree(vector const& v)
{
    boost::property_tree::ptree root;
    root.put("x", v[0]);
    root.put("y", v[1]);
    return root;
}

boost::property_tree::ptree get_ptree(std::shared_ptr<entity const> e)
{
    boost::property_tree::ptree root;
    root.put("id", e->id());
    root.put("type", e->type());
    root.add_child("pos", get_ptree(e->pos()));
    root.add_child("vel", get_ptree(e->dir()));
    return root;
}

boost::property_tree::ptree get_ptree(entities const& entities)
{
    boost::property_tree::ptree root;
    for (auto &p : entities)
        root.push_back(std::make_pair("", get_ptree(p.second)));
    return root;
}

std::string get_json(boost::property_tree::ptree const& root)
{
    std::stringstream ss;
    boost::property_tree::write_json(ss, root, false);
    std::string json = ss.str();
    if (*(json.cend() - 1) == '\n')
        json.erase(json.cend() - 1);
    if (*(json.cend() - 1) == '\r')
        json.erase(json.cend() - 1);
    return json;
}

std::string json_state_entities(entities const& entities)
{
    boost::property_tree::ptree root;
    root.add_child("data", get_ptree(entities));
    root.put("order", "state");
    root.put("suborder", "entities");
    return get_json(root);
}

std::string json_state_player(std::shared_ptr<entity const> e)
{
    boost::property_tree::ptree root = get_ptree(e);
    root.put("order", "state");
    root.put("suborder", "player");
    return get_json(root);
}
