#pragma once

#include <map>

#include "Entity.hpp"

class Behavior
{
public:
    virtual void update(Entity &self, std::map<int, Entity> &others, float delta) = 0;
};
typedef std::shared_ptr<Behavior> BehaviorPtr;

class Walk : public Behavior
{
private:
    float t_;
public:
    Walk();
    void update(Entity &self, std::map<int, Entity> &others, float delta);
};
