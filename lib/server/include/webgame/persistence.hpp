#pragma once

#include <memory>

#include "entities.hpp"

class persistence
{
public:
    virtual ~persistence() {};

    virtual void                    save(entities const& ents) = 0;
    virtual entities                load_all_npes() = 0;
    virtual std::shared_ptr<entity> load_player(std::string const& name) = 0;
    virtual void                    remove_all() = 0;
};
