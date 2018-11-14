#include "log.hpp"

namespace webgame {

bool io_log = false;
bool data_log = false;

#ifndef WEBGAME_MONOTHREAD
std::recursive_mutex log_mutex;
#endif /* !WEBGAME_MONOTHREAD */
unsigned int title_max_size = 50;
std::ostream *log_stream = &std::cout;

} // namespace webgame
