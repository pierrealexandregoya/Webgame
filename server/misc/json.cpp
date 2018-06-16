#include "json.hpp"

#include <boost/property_tree/json_parser.hpp>

#include "common.hpp"
#include "../entity.hpp"

boost::property_tree::ptree vec_to_ptree(vector const& v)
{
    boost::property_tree::ptree vec_node;
    vec_node.put("x", v[0]);
    vec_node.put("y", v[1]);
    return vec_node;
}

boost::property_tree::ptree entity_to_ptree(std::shared_ptr<entity const> e)
{
    boost::property_tree::ptree ptentity;
    ptentity.put("id", e->id());
    ptentity.put("type", e->type());
    ptentity.add_child("pos", vec_to_ptree(e->pos()));
    ptentity.add_child("vel", vec_to_ptree(e->dir()));
    return ptentity;
}

std::string entities_to_json(entities const& entities)
{
    boost::property_tree::ptree root;
    root.put("order", "state");
    root.put("suborder", "entities");
    boost::property_tree::ptree data;
    for (auto &p : entities)
    {
        auto &e = p.second;
        boost::property_tree::ptree ptentity = entity_to_ptree(e);
        data.push_back(std::make_pair("", ptentity));
    }
    root.add_child("data", data);
    std::ostringstream ss;
    boost::property_tree::write_json(ss, root, false);

    std::string msg = std::move(ss.str());
    assert(*(msg.cend() - 1) == '\n');
    auto it = msg.erase(msg.cend() - 1);
    assert(it == msg.cend());
    assert(*msg.rbegin() == '}');
    assert(msg.find("}\"") == std::string::npos);
    assert(msg.find("\"\"") == std::string::npos);
    return msg;
}
