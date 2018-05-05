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
            self_->setDir({ 1.f, 0.f });
        else if (dir == 1)
            self_->setDir({ 1.f, 1.f });
        else if (dir == 2)
            self_->setDir({ 0.f, 1.f });
        else if (dir == 3)
            self_->setDir({ -1.f, 1.f });
        else if (dir == 4)
            self_->setDir({ -1.f, 0.f });
        else if (dir == 5)
            self_->setDir({ -1.f, -1.f });
        else if (dir == 6)
            self_->setDir({ 0.f, -1.f });
        else if (dir == 7)
            self_->setDir({ 1.f, -1.f });
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
                self_->setDir(self_->dir() * -1.f);
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
