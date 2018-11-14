#pragma once

#include <functional>
#include <string>

#include <boost/beast/core/buffers_to_string.hpp>

#include "filesystem.hpp"
#include "nmoc.hpp"
#include "time.hpp"

namespace webgame {

class WEBGAME_API on_scope
{
public:
    typedef void f_proto();
    typedef std::function<f_proto>  f_type;

protected:
    f_type fin_;
    f_type fout_;

private:
    WEBGAME_NON_MOVABLE_OR_COPYABLE(on_scope);

public:
    on_scope(f_type fin, f_type fout);
    on_scope(f_type fout);
    ~on_scope();
};


class WEBGAME_API scope_time
{
private:
    steady_clock::time_point    in_time_;
    std::string                 msg_;

private:
    WEBGAME_NON_MOVABLE_OR_COPYABLE(scope_time);

public:
    scope_time(std::string const& msg = "");
    ~scope_time();
};

} // namespace webgame

// todo: maybe replace by a config macro like NO_TIME_LOG
#ifndef NDEBUG
# define _WEBGAME_TIME_SCOPE(msg) scope_time _my_st(msg) 

# define WEBGAME_TIME_SCOPE auto const _my_ln = __LINE__;\
_WEBGAME_TIME_SCOPE(filesystem::path(__FILE__).filename().string() + ":" + __FUNCTION__ + "():" + std::to_string(_my_ln))

# define WEBGAME_TIME_THIS(to_time) auto const _my_ln = __LINE__;\
do {\
_WEBGAME_TIME_SCOPE(#to_time);\
to_time;\
} while (0)

#else
# define WEBGAME_TIME_SCOPE 
//# define TIME_SCOPE do {} while (0)
# define WEBGAME_TIME_THIS(to_time) to_time
#endif /* !NDEBUG */

namespace webgame {

template<class ConstBufferSequence>
std::string get_readable(ConstBufferSequence const& bufs)
{
    std::ostringstream ss;
    for (auto const& b : bufs)
        for (int i = 0; i < b.size(); ++i)
        {
            char c = static_cast<char const*>(b.data())[i];
            if (isprint(c))
                if (c == '\\')
                    ss << "\\\\";
                else
                    ss << c;
            else
                ss << "\0x" << std::hex << static_cast<int>(static_cast<unsigned char>(c));
        }
    return ss.str();
}

template<class ConstBufferSequence>
std::string buffer_to_string(ConstBufferSequence const& bufs, size_t n)
{
    std::string str = boost::beast::buffers_to_string(bufs);
    str.resize(n);
    return str;
}

} // namespace webgame
