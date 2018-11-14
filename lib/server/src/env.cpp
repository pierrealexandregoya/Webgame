#include "env.hpp"

namespace webgame {

env::env(entities const & entities)
    : entities_(entities)
{}

entities & env::others()
{
    return entities_;
}

} // namespace webgame
