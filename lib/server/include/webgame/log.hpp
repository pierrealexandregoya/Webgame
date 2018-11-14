#pragma once

#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

#include "config.hpp"

namespace webgame {

WEBGAME_API extern bool io_log;
WEBGAME_API extern bool data_log;
#ifndef WEBGAME_MONOTHREAD
WEBGAME_API extern std::recursive_mutex log_mutex;
#endif /* !WEBGAME_MONOTHREAD */
WEBGAME_API extern unsigned int title_max_size;
WEBGAME_API extern std::ostream *log_stream;

} // namespace webgame


#if (!defined(WEBGAME_TESTS) && !defined(WEBGAME_NO_LOG)) || (defined(WEBGAME_TESTS) && !defined(NDEBUG))
# include "lock.hpp"
# ifdef WEBGAME_BUILDING_THE_LIB
#  define _WEBGAME_MY_LOG(to_log)\
do {\
WEBGAME_LOCK(log_mutex);\
*log_stream << to_log << std::endl;\
} while (0)
# else /* WEBGAME_BUILDING_THE_LIB */
#  define _WEBGAME_MY_LOG(to_log)\
do {\
WEBGAME_LOCK(webgame::log_mutex);\
*webgame::log_stream << to_log << std::endl;\
} while (0)
# endif /* WEBGAME_BUILDING_THE_LIB */
#else /* (!defined(WEBGAME_TESTS) && !defined(WEBGAME_NO_LOG)) || (defined(WEBGAME_TESTS) && !defined(NDEBUG)) */
# define _WEBGAME_MY_LOG(to_log) do {} while (0)
#endif /* (!defined(WEBGAME_TESTS) && !defined(WEBGAME_NO_LOG)) || (defined(WEBGAME_TESTS) && !defined(NDEBUG)) */

#ifdef WEBGAME_BUILDING_THE_LIB
# define WEBGAME_LOG(title, to_log) _WEBGAME_MY_LOG("[" << std::setw(5) << std::this_thread::get_id() << "] [" << std::setw(title_max_size) << title << "] " << to_log)
#else /* WEBGAME_BUILDING_THE_LIB */
# define WEBGAME_LOG(title, to_log) _WEBGAME_MY_LOG("[" << std::setw(5) << std::this_thread::get_id() << "] [" << std::setw(webgame::title_max_size) << title << "] " << to_log)
#endif /* WEBGAME_BUILDING_THE_LIB */
