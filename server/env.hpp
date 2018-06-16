#pragma once

#include "misc/containers.hpp"

class env
{
private:
    entities entities_;

public:
    env(entities const & entities);

public:
    entities & others();
};
