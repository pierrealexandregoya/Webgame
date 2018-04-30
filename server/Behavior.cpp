#include <iostream>

#include "Behavior.hpp"
#include "Entity.hpp"

Behavior::Behavior()
    : resolved_(false)
    , self_(nullptr)
{}

void Behavior::setSelf(Entity * self)
{
    self_ = self;
}

Walk::Walk()
    : Behavior()
    , t_(0)
{}

bool Walk::update(float delta, Env &env)
{
    assert(self_ != nullptr);
    resolved_ = true;
    t_ += delta;
    if (t_ >= 3.)
    {
        // Better: %2, then rotate dir_ by one degree
        auto dir = ::rand() % 8;
        if (dir == 0)
            self_->setVel(Array2({ 1., 0. }));
        else if (dir == 1)
            self_->setVel(Array2({ 1., 1. }));
        else if (dir == 2)
            self_->setVel(Array2({ 0., 1. }));
        else if (dir == 3)
            self_->setVel(Array2({ -1., 1. }));
        else if (dir == 4)
            self_->setVel(Array2({ -1., 0. }));
        else if (dir == 5)
            self_->setVel(Array2({ -1., -1. }));
        else if (dir == 6)
            self_->setVel(Array2({ 0., -1. }));
        else if (dir == 7)
            self_->setVel(Array2({ 1., -1. }));
        t_ = 0.;
    }
    return resolved_;
}

AreaLimit::AreaLimit(AreaType areaType, int size)
    : Behavior()
    , areaType_(areaType)
    , size_(size)
{}

bool AreaLimit::update(float delta, Env &env)
{
    assert(self_ != nullptr);
    if (areaType_ == Square)
    {
        if (self_->pos()[0] > size_ || self_->pos()[0] < -size_ || self_->pos()[1] > size_ || self_->pos()[1] < -size_)
        {
            if (resolved_)
                self_->setVel(self_->vel() * -1);
            resolved_ = false;
        }
        else
            resolved_ = true;
    }
    else
    {
        std::cout << "AreaLimit: unexpected type: " << std::to_string(areaType_) << std::endl;
        resolved_ = true;
    }
    return resolved_;
}
