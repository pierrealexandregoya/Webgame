#include "Entity.hpp"

Entity::Entity(Array2 const& pos, Array2 const& vel, std::string const& type)
    : id_(::rand())
    , pos_(2)
    , vel_(2)
    , type_(type)
{
    for (int i = 0; i < 2; ++i)
    {
        pos_[i] = pos[i];
        vel_[i] = vel[i];
    }
}

void Entity::update(float d)
{
    if (rand() % 2 == 0)
        pos_ += vel_ * d;
    else
        pos_ -= vel_ * d;
}

VecType const& Entity::pos() const
{
    return pos_;
}

VecType const& Entity::vel() const
{
    return vel_;
}

int Entity::id() const
{
    return id_;
}

std::string const& Entity::type() const
{
    return type_;
}
