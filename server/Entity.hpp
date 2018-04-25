#pragma once

#include "Server.hpp"

class Entity
{
private:
    int id_;
    VecType pos_;
    VecType vel_;
    std::string type_;

public:
    Entity(Array2 const& pos, Array2 const& vel, std::string const& type);

    void                update(float d);
    VecType const&      pos() const;
    VecType const&      vel() const;
    int                 id() const;
    std::string const&  type() const;
};
