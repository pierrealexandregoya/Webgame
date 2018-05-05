#pragma once

#include <map>

#include "types.hpp"

class Entity: public std::enable_shared_from_this<Entity>
{
    friend Behavior;
    friend Walk;
    friend AreaLimit;
private:
    int                     id_;
    Vector                  pos_;
    Vector                  dir_;
    std::string             type_;
    Behaviors               behaviors_;
    float                   speed_;

    NON_MOVABLE_OR_COPYABLE(Entity);

public:
    //Entity(Entity &&other) = default;
    Entity() = default;
    Entity(Vector const& pos, Vector const& dir, float speed, std::string const& type, Behaviors && behaviors=Behaviors());

    void                update(float d, Env & env);
    void                treatBehaviors(float d, Env & env);
    Vector const&       pos() const;
    Vector const&       dir() const;
    void                setDir(Vector const& vec);
    int                 id() const;
    std::string const&  type() const;
};
