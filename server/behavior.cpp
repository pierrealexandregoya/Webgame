#include "behavior.hpp"

#include <iomanip>
#include <iostream>

#include "entity.hpp"
#include "env.hpp"
#include "log.hpp"
#include "random.hpp"
#include "vector.hpp"

#ifndef NDEBUG
bool behavior::operator==(behavior const& other)
{
    return resolved_ == other.resolved_;
}

bool behavior::operator!=(behavior const& other)
{
    return !(*this == other);
}
#endif /* !NDEBUG */

behavior::behavior()
    : resolved_(false)
    , self_(nullptr)
{}

behavior::~behavior()
{}

void behavior::set_self(entity * self)
{
    self_ = self;
}

bool const& behavior::resolved() const
{
    return resolved_;
}

// WALK AROUND
#ifndef NDEBUG
bool walkaround::operator==(behavior const& o)
{
    walkaround const& other = dynamic_cast<decltype(other)>(o);
    return behavior::operator==(other)
        && t_ == other.t_;
}

bool walkaround::operator!=(behavior const& other)
{
    return !(*this == other);
}
#endif /* !NDEBUG */

walkaround::walkaround()
    : behavior()
    , t_(0)
{}

void walkaround::update(duration delta, env &env)
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

// AREA LIMIT
#ifndef NDEBUG
bool arealimit::operator==(behavior const& o)
{
    arealimit const& other = dynamic_cast<decltype(other)>(o);
    return behavior::operator==(other)
        && area_type_ == other.area_type_
        && radius_ == other.radius_
        && center_ == other.center_;
}
bool arealimit::operator!=(behavior const& other)
{
    return !(*this == other);
}
#endif /* !NDEBUG */

arealimit::arealimit(area_type area_type, real radius, vector const& center)
    : behavior()
    , area_type_(area_type)
    , radius_(radius)
    , center_(center)
{}

void arealimit::update(duration delta, env &env)
{
    assert(self_ != nullptr);
    if (area_type_ == Square)
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
        LOG("BEHAVIOR", "arealimit: unexpected type: " << static_cast<unsigned int>(area_type_));
        resolved_ = true;
    }
}

// ATTACK ON SIGHT
#ifndef NDEBUG
bool attack_on_sight::operator==(behavior const& o)
{
    attack_on_sight const& other = dynamic_cast<decltype(other)>(o);
    return behavior::operator==(other)
        && radius_ == other.radius_;
}
bool attack_on_sight::operator!=(behavior const& other)
{
    return !(*this == other);
}
#endif /* !NDEBUG */

attack_on_sight::attack_on_sight(real radius)
    : behavior()
    , radius_(radius)
{}

void attack_on_sight::update(duration delta, env &env)
{
    assert(self_ != nullptr);

    real enemy_dist = 0;
    std::shared_ptr<entity const> closest_enemy;
    for (auto const& e : env.others())
    {
        if (e.second->type() == self_->type() || e.second->type().find("object") != std::string::npos)
            continue;
        real dist = ublas::norm_2(self_->pos() - e.second->pos());
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

// STOP
void stop::update(duration delta, env &env)
{
    self_->set_speed(0.f);
    resolved_ = true;
}
