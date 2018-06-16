#include "utils.hpp"

#include <sstream>

#include <boost/beast/core/multi_buffer.hpp>

#include "log.hpp"

namespace beast = boost::beast;

on_scope::on_scope(f_type fin, f_type fout)
    : fin_(fin)
    , fout_(fout)
{
    fin_();
}

on_scope::on_scope(f_type fout)
    : on_scope([] {}, fout)
{}

on_scope::~on_scope()
{
    fout_();
}


scope_time::scope_time(std::string const& msg)
    : in_time_(steady_clock::now())
    , msg_(msg)
{
    std::string title = " TIME";
    size_t max_size = title_max_size - title.size();

    if (msg_.length() > max_size)
        msg_.replace(max_size, msg_.length() - max_size, ""/*"..."*/);
    else
        msg_ += std::string(max_size - msg_.length(), ' ');
    msg_ += " TIME";

    LOG(msg_, std::chrono::duration_cast<readable_duration>(in_time_ - start_time).count());
}

scope_time::~scope_time()
{
    steady_clock::time_point out_time = steady_clock::now();

    LOG(msg_, std::chrono::duration_cast<readable_duration>(out_time - start_time).count() << " (+" << std::chrono::duration_cast<readable_duration>(out_time - in_time_).count() << ")");
}

std::string get_readable(boost::beast::multi_buffer::const_buffers_type const& bufs)
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
