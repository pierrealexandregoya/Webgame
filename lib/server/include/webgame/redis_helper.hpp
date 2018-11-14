#pragma once

#include <memory>

#include <bredis/Connection.hpp>
#include <bredis/Extract.hpp>

#include "config.hpp"
#include "nmoc.hpp"

namespace webgame {

class WEBGAME_API redis_helper : public std::enable_shared_from_this<redis_helper>
{
private:
    bredis::Connection<boost::asio::ip::tcp::socket>    socket_;
    boost::asio::streambuf                              read_buffer_;
    boost::asio::streambuf                              write_buffer_;
    std::list<std::function<void()>>                    tasks_;
#ifndef WEBGAME_MONOTHREAD
    std::recursive_mutex                                tasks_mutex_;
#endif /* !WEBGAME_MONOTHREAD */

public:
    WEBGAME_NON_MOVABLE_OR_COPYABLE(redis_helper);

    redis_helper(boost::asio::ip::tcp::socket &socket);

public:
    void select(unsigned int index);

    void flushdb();

    size_t dbsize();

    std::vector<std::string> keys(std::string const& pattern);

    void async_set(std::string const& key, std::string const& value, std::function<void()> &&handler);
    void async_get(std::string const& key, std::function<void(bool, std::string&&)> &&handler);

    std::vector<std::string> multi_get(std::vector<std::string> const& keys);

    void async_multi_set(std::vector<std::pair<std::string, std::string>> const& keys_values, std::function<void()> &&handler);

private:
    void task_set(std::string const& key, std::string const& value, std::function<void()> &handler);
    void task_get(std::string const& key, std::function<void(bool, std::string&&)> &handler);
    void task_multi_set(std::vector<std::pair<std::string, std::string>> const& keys_values, std::function<void()> &handler);
    void exec_next();

    void command_result_str(bredis::single_command_t const& cmd, std::string const& expected_res);
    static void check_extract_str(bredis::extracts::extraction_result_t const& extract, std::string const& expected_res);
    static std::string command_to_string(bredis::single_command_t const& cmd);
};

} // namespace webgame
