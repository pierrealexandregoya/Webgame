#include "random.hpp"

namespace webgame {

std::random_device rd;  //Will be used to obtain a seed for the random number engine
std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
std::uniform_int_distribution<unsigned int> max_rand(0, std::numeric_limits<unsigned int>::max());
std::uniform_int_distribution<unsigned int> &id_rand = max_rand;
std::uniform_int_distribution<unsigned int> dir_rand(0, 8);

} // namespace webgame
