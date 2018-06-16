#include "log.hpp"
#include "random.hpp"
#include "time.hpp"

bool io_log = false;
bool data_log = false;

std::recursive_mutex log_mutex;
unsigned int title_max_size = 50;

std::random_device rd;  //Will be used to obtain a seed for the random number engine
std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
std::uniform_int_distribution<unsigned int> max_rand(0, std::numeric_limits<unsigned int>::max());
std::uniform_int_distribution<unsigned int> &id_rand = max_rand;
std::uniform_int_distribution<unsigned int> dir_rand(0, 8);

steady_clock::time_point start_time = steady_clock::now();
