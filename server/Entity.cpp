#include "Entity.hpp"
#include "Behavior.hpp"

Entity::Entity(Array2 const& pos, Array2 const& vel, std::string const& type, std::initializer_list<Behavior*> behaviors)
    : id_(::rand())
    , pos_(2)
    , vel_(2)
    , type_(type)
{
    behaviors_.reserve(10);
    for (auto b : behaviors)
        behaviors_.push_back(b);

    for (int i = 0; i < 2; ++i)
    {
        pos_[i] = pos[i];
        vel_[i] = vel[i];
    }
}

void Entity::update(float d)
{
    std::map<int, Entity> fake;
    for (auto b : behaviors_)
        b->update(*this, fake, d);
    //if (rand() % 2 == 0)
    //    vel_ *= -1;

    if (type_ == "player")
    {
        //std::cout << "PLAYER " << id_ << " UPDATE:" << std::endl;
        //std::cout << "\tpos  : " << pos_[0] << ", " << pos_[1] << std::endl;
        //std::cout << "\tvel  : " << vel_[0] << ", " << vel_[1] << std::endl;
        //std::cout << "\t|vel|: " << boost::numeric::ublas::norm_2(vel_) << std::endl;
        //std::cout << "\td    : " << d << std::endl;
    }
    pos_ += vel_ * d;
}

VecType const& Entity::pos() const
{
    return pos_;
}

VecType const& Entity::vel() const
{
    return vel_;
}

void Entity::setVel(Array2 const& vel)
{
    vel_[0] = vel[0];
    vel_[1] = vel[1];
}

void Entity::setVel(VecType const& vel)
{
    vel_ = vel;
}

int Entity::id() const
{
    return id_;
}

std::string const& Entity::type() const
{
    return type_;
}
