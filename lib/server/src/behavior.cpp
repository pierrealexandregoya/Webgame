#include "behavior.hpp"

#include <iomanip>
#include <iostream>

#include "env.hpp"
#include "log.hpp"
#include "npc.hpp"
#include "random.hpp"
#include "save_load.hpp"
#include "vector.hpp"

namespace webgame {

//-----------------------------------------------------------------------------
// BEHAVIOR

behavior::behavior()
    : resolved_(false)
    , self_(nullptr)
{}

behavior::~behavior()
{}

nlohmann::json behavior::save() const
{
    return { {"resolved", resolved_ } };
}

void behavior::load(nlohmann::json const& j)
{
    if (!j.is_object()
        || !j.count("resolved")
        || !j["resolved"].is_boolean())
        throw std::runtime_error("behavior: invalid JSON");

    resolved_ = j["resolved"].get<bool>();
}

void behavior::set_self(npc * self)
{
    self_ = self;
}

bool const& behavior::resolved() const
{
    return resolved_;
}

#ifdef WEBGAME_TESTS
bool behavior::operator==(behavior const& other) const
{
    return resolved_ == other.resolved_;
}

bool behavior::operator!=(behavior const& other) const
{
    return !(*this == other);
}
#endif /* WEBGAME_TESTS */

//-----------------------------------------------------------------------------
// WALKAROUND

walkaround::walkaround()
    : behavior()
    , t_(0)
{}

void walkaround::update(double delta, env &env)
{
    assert(self_ != nullptr);
    resolved_ = true;
    t_ += delta;
    if (t_ >= 0.5f)
    {
        // Better: %2, then rotate dir_ by one degree
        auto dir = dir_rand(gen);
        if (dir == 0)
            self_->set_dir({ 1.f, 0.f });
        else if (dir == 1)
            self_->set_dir({ 1.f, 1.f });
        else if (dir == 2)
            self_->set_dir({ 0.f, 1.f });
        else if (dir == 3)
            self_->set_dir({ -1.f, 1.f });
        else if (dir == 4)
            self_->set_dir({ -1.f, 0.f });
        else if (dir == 5)
            self_->set_dir({ -1.f, -1.f });
        else if (dir == 6)
            self_->set_dir({ 0.f, -1.f });
        else if (dir == 7)
            self_->set_dir({ 1.f, -1.f });
        t_ = 0.;
    }
}

nlohmann::json walkaround::save() const
{
    return {
        {"behavior", behavior::save()},
        {"t", t_},
        {"type", "walkaround"}
    };
}

void walkaround::load(nlohmann::json const& j)
{
    if (!j.is_object() || !j.count("behavior") || !j.count("t")
        || !j["t"].is_number_float())
        throw std::runtime_error("walkaround: invalid JSON");

    behavior::load(j["behavior"]);
    t_ = j["t"].get<double>();
}

#ifdef WEBGAME_TESTS
bool walkaround::operator==(behavior const& o) const
{
    walkaround const& other = dynamic_cast<decltype(other)>(o);
    return behavior::operator==(other)
        && t_ == other.t_;
}

bool walkaround::operator!=(behavior const& other) const
{
    return !(*this == other);
}
#endif /* WEBGAME_TESTS */

WEBGAME_REGISTER(behavior, walkaround);

//-----------------------------------------------------------------------------
// AREALIMIT

arealimit::arealimit(area_type area_type, double radius, vector const& center)
    : behavior()
    , area_type_(area_type)
    , radius_(radius)
    , center_(center)
{}

void arealimit::update(double delta, env &env)
{
    assert(self_ != nullptr);
    if (area_type_ == square)
    {
        if (self_->pos()[0] > center_[0] + radius_ || self_->pos()[0] < center_[0] - radius_ || self_->pos()[1] > center_[1] + radius_ || self_->pos()[1] < center_[1] - radius_)
        {
            self_->set_speed(self_->max_speed());
            self_->set_dir(center_ - self_->pos());
            resolved_ = false;
        }
        else
            resolved_ = true;
    }
    else
    {
        WEBGAME_LOG("BEHAVIOR", "arealimit: unexpected type: " << static_cast<unsigned int>(area_type_));
        resolved_ = true;
    }
}

nlohmann::json arealimit::save() const
{
    return {
        {"behavior", behavior::save()},
        {"area_type", area_type_},
        {"radius", radius_},
        {"center", center_.save()},
        {"type", "arealimit"}
    };
}

void arealimit::load(nlohmann::json const& j)
{
    if (!j.is_object() || !j.count("behavior") || !j.count("area_type")
        || !j["area_type"].is_number() || !j.count("radius")
        || !j["radius"].is_number_float() || !j.count("center"))
        throw std::runtime_error("arealimit: invalid JSON");

    behavior::load(j["behavior"]);
    area_type_ = j["area_type"].get<area_type>();
    radius_ = j["radius"].get<double>();
    center_.load(j["center"]);
}

#ifdef WEBGAME_TESTS
bool arealimit::operator==(behavior const& o) const
{
    arealimit const& other = dynamic_cast<decltype(other)>(o);
    return behavior::operator==(other)
        && area_type_ == other.area_type_
        && radius_ == other.radius_
        && center_ == other.center_;
}
bool arealimit::operator!=(behavior const& other) const
{
    return !(*this == other);
}
#endif /* WEBGAME_TESTS */

WEBGAME_REGISTER(behavior, arealimit);

//-----------------------------------------------------------------------------
// ATTACK ON SIGHT

attack_on_sight::attack_on_sight(double radius)
    : behavior()
    , radius_(radius)
{}

void attack_on_sight::update(double delta, env &env)
{
    assert(self_ != nullptr);

    double enemy_dist = 0;
    std::shared_ptr<located_entity const> closest_enemy;
    for (auto const& e : static_cast<entity_container<located_entity>>(env.others()))
    {
        if (e.second->type() == self_->type() || e.second->type().find("object") != std::string::npos)
            continue;
        double dist = vector((self_->pos() - e.second->pos())).norm();
        if (dist > radius_ || (closest_enemy && dist >= enemy_dist))
            continue;

        enemy_dist = dist;
        closest_enemy = e.second;
    }

    if (closest_enemy)
    {
        self_->set_speed(enemy_dist <= 0.15f ? 0.f : self_->max_speed());
        self_->set_dir(closest_enemy->pos() - self_->pos());
    }

    resolved_ = !closest_enemy;
}

nlohmann::json attack_on_sight::save() const
{
    return {
        {"behavior", behavior::save()},
        {"radius", radius_},
        {"type", "attack_on_sight"}
    };
}

void attack_on_sight::load(nlohmann::json const& j)
{
    if (!j.is_object() || !j.count("behavior") || !j.count("radius")
        || !j["radius"].is_number_float())
        throw std::runtime_error("arealimit: invalid JSON");

    behavior::load(j["behavior"]);
    radius_ = j["radius"].get<double>();
}

#ifdef WEBGAME_TESTS
bool attack_on_sight::operator==(behavior const& o) const
{
    attack_on_sight const& other = dynamic_cast<decltype(other)>(o);
    return behavior::operator==(other)
        && radius_ == other.radius_;
}
bool attack_on_sight::operator!=(behavior const& other) const
{
    return !(*this == other);
}
#endif /* WEBGAME_TESTS */

WEBGAME_REGISTER(behavior, attack_on_sight);

//-----------------------------------------------------------------------------
// STOP

void stop::update(double delta, env &env)
{
    self_->set_speed(0.f);
    resolved_ = true;
}

nlohmann::json stop::save() const
{
    return {
        {"behavior", behavior::save()},
        {"type", "stop"}
    };
}

void stop::load(nlohmann::json const& j)
{
    if (!j.is_object() || !j.count("behavior"))
        throw std::runtime_error("stop: invalid JSON");

    behavior::load(j["behavior"]);
}

WEBGAME_REGISTER(behavior, stop);

} // namespace webgame
