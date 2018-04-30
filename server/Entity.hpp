#pragma once

#include <map>

#include "types.hpp"

class Entity: public std::enable_shared_from_this<Entity>
{
    friend Behavior;
    friend Walk;
    friend AreaType;
private:
    int                     id_;
    VecType                 pos_;
    VecType                 dir_; // vec dir_; float speed ?
    std::string             type_;
    Behaviors               behaviors_;
    float                   speed_;

    NON_MOVABLE_OR_COPYABLE(Entity);

public:
    //Entity(Entity &&other) = default;
    Entity() = default;
    Entity(Array2 const& pos, Array2 const& vel, float speed, std::string const& type, Behaviors && behaviors=Behaviors());

    void                update(float d, Env & env);
    void                treatBehaviors(float d, Env & env);
    VecType const&      pos() const;
    VecType const&      vel() const;
    void                setVel(Array2 const& vec);
    void                setVel(VecType const& vec);
    int                 id() const;
    std::string const&  type() const;
};
