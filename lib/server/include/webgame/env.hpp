#pragma once

#include "config.hpp"
#include "entities.hpp"

namespace webgame {

class WEBGAME_API env
{
private:
    entities entities_;

public:
    env(entities const & entities);

public:
    entities & others();
};

} // namespace webgame
