#include "entity.hpp"

#include "behavior.hpp"
#include "random.hpp"

#ifndef NDEBUG
bool entity::operator==(entity const& other)
{
    if (id_ != other.id_
        || pos_ != other.pos_
        || dir_ != other.dir_
        || type_ != other.type_
        || speed_ != other.speed_
        || max_speed_ != other.max_speed_)
        return false;

    if (behaviors_.size() != other.behaviors_.size())
        return false;

    auto it1 = behaviors_.cbegin();
    auto it2 = other.behaviors_.cbegin();

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

bool entity::operator!=(entity const& other)
{
    return !(*this == other);
}
#endif /* !NDEBUG */

entity::entity(vector const& pos, vector const& dir, real speed, real max_speed, std::string const& type, behaviors_t && behaviors)
    : id_(id_rand(gen))
    , pos_()
    , dir_()
    , type_(type)
    , behaviors_(std::move(behaviors))
    , speed_(std::min(speed, max_speed))
    , max_speed_(max_speed)
{
    // vec_ = vec should suffise
    for (int i = 0; i < 2; ++i)
    {
        pos_[i] = pos[i];
        dir_[i] = dir[i];
    }

    init_behaviors();
}

entity::entity(entity && other)
    : id_(std::move(other.id_))
    , pos_(std::move(other.pos_))
    , dir_(std::move(other.dir_))
    , type_(std::move(other.type_))
    , behaviors_(std::move(other.behaviors_))
    , speed_(std::move(other.speed_))
    , max_speed_(std::move(other.max_speed_))
{
    init_behaviors();
}

void entity::init_behaviors()
{
    for (auto p : behaviors_)
        p.second->set_self(this);
}

bool entity::update(real d, env & env)
{
    vector prev_pos = pos_;
    vector prev_dir = dir_;
    treatbehaviors(d, env);

    auto norm = ublas::norm_2(dir_);
    if (::fabsf(norm) > std::numeric_limits<real>::epsilon() && ::fabsf(speed_) > std::numeric_limits<real>::epsilon())
    {
        dir_[0] = dir_[0] / norm;
        dir_[1] = dir_[1] / norm;
        pos_ = pos_ + dir_ * speed_ * d;
        assert(!std::isnan(dir_[0]));
        assert(!std::isnan(dir_[1]));
        assert(!std::isnan(pos_[0]));
        assert(!std::isnan(pos_[1]));
        assert(!std::isnan(speed_));
    }
    return prev_pos != pos_ || prev_dir != dir_;
}

void entity::treatbehaviors(real d, env & env)
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

void entity::set_dir(vector const& dir)
{
    dir_ = dir;
}

void entity::set_speed(real speed)
{
    speed_ = std::min(speed, max_speed_);
}

id_t const& entity::id() const
{
    return id_;
}

vector const& entity::pos() const
{
    return pos_;
}

vector const& entity::dir() const
{
    return dir_;
}

std::string const& entity::type() const
{
    return type_;
}

real const& entity::speed() const
{
    return speed_;
}

real const& entity::max_speed() const
{
    return max_speed_;
}
