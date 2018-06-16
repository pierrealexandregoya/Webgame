#include "env.hpp"

env::env(entities const & entities)
    : entities_(entities)
{}

entities & env::others()
{
    return entities_;
}
