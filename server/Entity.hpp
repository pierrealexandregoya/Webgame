#pragma once

#include "Server.hpp"

class Behavior;

class Entity
{
private:
    int                     id_;
    VecType                 pos_;
    VecType                 vel_;
    std::string             type_;
    std::vector<Behavior*>  behaviors_;

public:
    Entity() = default;
    Entity(Array2 const& pos, Array2 const& vel, std::string const& type, std::initializer_list<Behavior*> behaviors = {});

    void                update(float d);
    VecType const&      pos() const;
    VecType const&      vel() const;
    void                setVel(Array2 const& vec);
    void                setVel(VecType const& vec);
    int                 id() const;
    std::string const&  type() const;
};
typedef std::shared_ptr<Entity> EntityPtr;