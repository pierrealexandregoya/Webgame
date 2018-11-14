#pragma

#ifndef WEBGAME_MONOTHREAD
# include <mutex>
# define WEBGAME_LOCK(m) std::lock_guard<decltype(m)> _my_lock(m)
#else /* !WEBGAME_MONOTHREAD */
# define WEBGAME_LOCK(m) do {} while(0)
#endif /* !WEBGAME_MONOTHREAD */
