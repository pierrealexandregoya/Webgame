#include "npc.hpp"

#include "save_load.hpp"

namespace webgame {

npc::npc(std::string const& type, vector const& pos, vector const& dir, double speed, double max_speed, behaviors && behaviors)
    : mobile_entity(type, pos, dir, speed, max_speed)
    , behaviors_(std::move(behaviors))
{
    init_behaviors();
}

bool npc::update(double d, env & env)
{
    vector prev_pos = pos_;
    vector prev_dir = dir_;

    treatbehaviors(d, env);
    bool has_changed = prev_pos != pos_ || prev_dir != dir_;

    return mobile_entity::update(d, env) || has_changed;
}

nlohmann::json npc::save() const
{
    nlohmann::json j;
    j["mobile_entity"] = mobile_entity::save();
    for (auto bhvr : behaviors_)
        j["behaviors"].emplace_back(nlohmann::json::array({ bhvr.first, bhvr.second->save() }));
    j["type"] = "npc";
    return j;
}

void npc::load(nlohmann::json const& j)
{
    if (!j.is_object() || !j.count("mobile_entity") || !j.count("behaviors")
        || !j["behaviors"].is_array())
        throw std::runtime_error("npc: invalid JSON");

    mobile_entity::load(j["mobile_entity"]);

    for (auto const& bhvr : j["behaviors"])
    {
        if (!bhvr.is_array() || bhvr.size() != 2
            || !bhvr[0].is_number_integer())
            throw std::runtime_error("npc: invalid JSON");
        behaviors_.insert(std::make_pair(bhvr[0].get<int>(), load_behavior(bhvr[1])));
    }

    init_behaviors();
}

void npc::init_behaviors()
{
    for (auto p : behaviors_)
        p.second->set_self(this);
}

void npc::treatbehaviors(double d, env & env)
{
    if (behaviors_.empty())
        return;

    bool resolved = true;

    int priority = behaviors_.begin()->first;

    for (auto const& p : behaviors_)
    {
        if (p.first > priority && !resolved)
            break;
        priority = p.first;
        p.second->update(d, env);
        resolved = p.second->resolved();
    }
}

#ifdef WEBGAME_TESTS
bool npc::operator==(entity const& other) const
{
    if (!mobile_entity::operator==(other))
        return false;

    npc const& true_other = static_cast<decltype(true_other)>(other);

    if (behaviors_.size() != true_other.behaviors_.size())
        return false;

    auto it1 = behaviors_.cbegin();
    auto it2 = true_other.behaviors_.cbegin();

    while (it1 != behaviors_.cend())
    {
        if (it1->first != it2->first
            || *it1->second != *it2->second)
            return false;
        ++it1;
        ++it2;
    }

    return true;
}

bool npc::operator!=(entity const& other) const
{
    return !(*this == other);
}
#endif /* WEBGAME_TESTS */

WEBGAME_REGISTER(entity, npc);

} // namespace webgame
