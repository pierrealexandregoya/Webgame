#include "entity.hpp"

#include "behavior.hpp"
#include "misc/random.hpp"

entity::entity(vector const& pos, vector const& dir, real speed, real max_speed, std::string const& type, behaviors && behaviors)
    : id_(id_rand(gen))
    , pos_()
    , dir_()
    , type_(type)
    , behaviors_(std::move(behaviors))
    , speed_(std::min(speed, max_speed))
    , max_speed_(max_speed)
{
    for (int i = 0; i < 2; ++i)
    {
        pos_[i] = pos[i];
        dir_[i] = dir[i];
    }
    for (auto p : behaviors_)
        p.second->set_self(this);
}

void entity::update(real d, env & env)
{
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
}

void entity::treatbehaviors(real d, env & env)
{
    if (behaviors_.empty())
        return;

    bool r = true;

    real k = 0;
    // testme
    for (auto const& p : behaviors_)
    {
        if (p.first - k > std::numeric_limits<real>::epsilon() && !r)
            break;
        p.second->update(d, env);
        if (!p.second->resolved())
            r = false;
    }
}

void entity::set_dir(vector const& dir)
{
    dir_ = dir;
}

void entity::set_speed(real speed)
{
    speed_ = speed;
    if (speed_ > max_speed_)
        speed_ = max_speed_;
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
