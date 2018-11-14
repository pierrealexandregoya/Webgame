#pragma once

#include <random>

namespace webgame {

extern std::random_device                           rd;
extern std::mt19937                                 gen;
extern std::uniform_int_distribution<unsigned int>  max_rand;
extern std::uniform_int_distribution<unsigned int>  &id_rand;
extern std::uniform_int_distribution<unsigned int>  dir_rand;

} // namespace webgame
