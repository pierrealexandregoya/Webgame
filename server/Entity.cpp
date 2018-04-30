#include <numeric>

#include "Behavior.hpp"
#include "Entity.hpp"

Entity::Entity(Array2 const& pos, Array2 const& vel, float speed, std::string const& type, Behaviors && behaviors)
    : id_(::rand())
    , pos_(2)
    , dir_(2)
    , type_(type)
    , behaviors_(std::move(behaviors))
    , speed_(speed)
{
    for (int i = 0; i < 2; ++i)
    {
        pos_[i] = pos[i];
        dir_[i] = vel[i];
    }
    for (auto p : behaviors_)
        p.second->setSelf(this);
}

void Entity::update(float d, Env & env)
{
    treatBehaviors(d, env);
    //if (rand() % 2 == 0)
    //    dir_ *= -1;

    if (type_ == "player")
    {
        //std::cout << "PLAYER " << id_ << " UPDATE:" << std::endl;
        //std::cout << "\tpos  : " << pos_[0] << ", " << pos_[1] << std::endl;
        //std::cout << "\tvel  : " << dir_[0] << ", " << dir_[1] << std::endl;
        //std::cout << "\t|vel|: " << boost::numeric::ublas::norm_2(dir_) << std::endl;
        //std::cout << "\td    : " << d << std::endl;
    }

    auto norm = boost::numeric::ublas::norm_2(dir_);
    if (::fabsf(norm) > std::numeric_limits<float>::epsilon() && ::fabsf(speed_) > std::numeric_limits<float>::epsilon())
    {
        dir_[0] = dir_[0] / norm;
        dir_[1] = dir_[1] / norm;
        pos_ += dir_ * speed_ * d;
        assert(!std::isnan(dir_[0]));
        assert(!std::isnan(dir_[1]));
        assert(!std::isnan(pos_[0]));
        assert(!std::isnan(pos_[1]));
        assert(!std::isnan(speed_));
    }
}

void Entity::treatBehaviors(float d, Env & env)
{
    if (behaviors_.empty())
        return;

    std::map<int, P<Entity>> fake;
    bool r = true;

    float k = 0;
    for (auto const& p : behaviors_)
    {
        if (p.first - k > std::numeric_limits<float>::epsilon() && !r)
            break;
        if (!p.second->update(d, env))
            r = false;
    }
}

VecType const& Entity::pos() const
{
    return pos_;
}

VecType const& Entity::vel() const
{
    return dir_;
}

void Entity::setVel(Array2 const& vel)
{
    dir_[0] = vel[0];
    dir_[1] = vel[1];
}

void Entity::setVel(VecType const& vel)
{
    dir_ = vel;
}

int Entity::id() const
{
    return id_;
}

std::string const& Entity::type() const
{
    return type_;
}
