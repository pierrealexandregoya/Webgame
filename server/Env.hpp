#pragma once

#include "types.hpp"

class Env
{
private:
    P<Entities> entities_;
public:
    Env(P<Entities> const& entities);
};