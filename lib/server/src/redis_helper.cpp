#include "redis_helper.hpp"

#include <bredis/Extract.hpp>
#include <bredis/MarkerHelpers.hpp>

#include "lock.hpp"

using buffer_t = boost::asio::streambuf;
using it_t = typename bredis::to_iterator<buffer_t>::iterator_t;
using policy_t = bredis::parsing_policy::keep_result;
using result_t = bredis::parse_result_mapper_t<it_t, policy_t>;

namespace webgame {

redis_helper::redis_helper(boost::asio::ip::tcp::socket &socket)
    : socket_(std::move(socket))
{}

void redis_helper::select(unsigned int idx)
{
    command_result_str(bredis::single_command_t({ "SELECT", std::to_string(idx) }), "OK");
}

void redis_helper::flushdb()
{
    command_result_str(bredis::single_command_t({ "FLUSHDB" }), "OK");
}

size_t redis_helper::dbsize()
{
    socket_.write(bredis::single_command_t({ "DBSIZE" }));
    result_t res = socket_.read(read_buffer_);
    bredis::extracts::extraction_result_t extract = boost::apply_visitor(bredis::extractor<it_t>(), res.result);
    read_buffer_.consume(res.consumed);

    int dbsize;
    try {
        dbsize = boost::get<bredis::extracts::int_t>(extract);
    }
    catch (...) {
        throw std::runtime_error("redis_helper: dbsize: result is not a integer");
    }
    if (dbsize < 0)
        throw std::runtime_error("redis_helper: dbsize: result is < 0: " + std::to_string(dbsize));
    return dbsize;
}

std::vector<std::string> redis_helper::keys(std::string const& pattern)
{
    socket_.write(bredis::single_command_t({ "KEYS", pattern }));
    result_t res = socket_.read(read_buffer_);
    bredis::extracts::extraction_result_t extract = boost::apply_visitor(bredis::extractor<it_t>(), res.result);
    read_buffer_.consume(res.consumed);
    bredis::extracts::array_holder_t &array = boost::get<bredis::extracts::array_holder_t>(extract);
    std::vector<std::string> keys;
    keys.reserve(array.elements.size());
    for (bredis::extracts::extraction_result_t &array_elem : array.elements)
        keys.emplace_back(std::move(boost::get<bredis::extracts::string_t>(array_elem).str));
    return keys;
}

void redis_helper::async_set(std::string const& key, std::string const& value, std::function<void()> &&handler)
{
    WEBGAME_LOCK(tasks_mutex_);
    tasks_.emplace_back(std::bind(&redis_helper::task_set, shared_from_this(), key, value, std::move(handler)));
    if (tasks_.size() == 1)
        tasks_.front()();
}

void redis_helper::async_get(std::string const& key, std::function<void(bool, std::string&&)> &&handler)
{
    WEBGAME_LOCK(tasks_mutex_);
    tasks_.emplace_back(std::bind(&redis_helper::task_get, shared_from_this(), key, std::move(handler)));
    if (tasks_.size() == 1)
        tasks_.front()();
}

std::vector<std::string> redis_helper::multi_get(std::vector<std::string> const& keys)
{
    bredis::command_container_t transaction;
    transaction.reserve(keys.size() + 2);
    transaction.emplace_back(bredis::single_command_t({ "MULTI" }));
    for (std::string const& key : keys)
        transaction.emplace_back(bredis::single_command_t({ "GET", key }));
    transaction.emplace_back(bredis::single_command_t({ "EXEC" }));

    socket_.write(transaction);

    result_t res = socket_.read(read_buffer_);
    bredis::extracts::extraction_result_t extract = boost::apply_visitor(bredis::extractor<it_t>(), res.result);
    read_buffer_.consume(res.consumed);
    check_extract_str(extract, "OK");

    for (size_t i = 0; i < keys.size(); ++i)
    {
        res = socket_.read(read_buffer_);
        extract = boost::apply_visitor(bredis::extractor<it_t>(), res.result);
        read_buffer_.consume(res.consumed);
        check_extract_str(extract, "QUEUED");
    }

    res = socket_.read(read_buffer_);
    extract = boost::apply_visitor(bredis::extractor<it_t>(), res.result);
    read_buffer_.consume(res.consumed);
    bredis::extracts::array_holder_t *trans_results_p;
    try {
        trans_results_p = &boost::get<bredis::extracts::array_holder_t>(extract);
    }
    catch (...) {
        throw std::runtime_error("redis_helper: multi_get: result is not an array");
    }
    if (trans_results_p->elements.size() != keys.size())
        throw std::runtime_error("redis_helper: multi_get: result array size is bad");

    std::vector<std::string> values;
    values.reserve(trans_results_p->elements.size());
    for (bredis::extracts::extraction_result_t & res_element : trans_results_p->elements)
    {
        try {
            values.emplace_back(std::move(boost::get<bredis::extracts::string_t>(res_element).str));
        }
        catch (...) {
            throw std::runtime_error("redis_helper: multi_get: result array element is not a string");
        }
    }

    return values;
}

void redis_helper::async_multi_set(std::vector<std::pair<std::string, std::string>> const& keys_values, std::function<void()> &&handler)
{
    WEBGAME_LOCK(tasks_mutex_);
    tasks_.emplace_back(std::bind(&redis_helper::task_multi_set, shared_from_this(), keys_values, std::move(handler)));
    if (tasks_.size() == 1)
        tasks_.front()();
}

void redis_helper::task_set(std::string const& key, std::string const& value, std::function<void()> &handler)
{
    auto this_p = shared_from_this();
    socket_.async_write(write_buffer_, bredis::single_command_t({ "SET", key, value }), [this_p, &handler](boost::system::error_code const& ec, std::size_t bytes_transferred) {
        if (ec)
            throw std::runtime_error("redis_helper: task_set: error during async write: " + ec.message());
        this_p->write_buffer_.consume(bytes_transferred);
        this_p->socket_.async_read(this_p->read_buffer_, [this_p, &handler](boost::system::error_code const& ec, result_t &&res) {
            if (ec)
                throw std::runtime_error("redis_helper: task_set: error during async read: " + ec.message());

            bredis::extracts::extraction_result_t extract = boost::apply_visitor(bredis::extractor<it_t>(), res.result);
            this_p->read_buffer_.consume(res.consumed);
            check_extract_str(extract, "OK");
            handler();
            this_p->exec_next();
        });
    });
}

void redis_helper::task_get(std::string const& key, std::function<void(bool, std::string&&)> &handler)
{
    auto this_p = shared_from_this();
    socket_.async_write(write_buffer_, bredis::single_command_t({ "GET", key }), [this_p, &handler](boost::system::error_code const& ec, std::size_t bytes_transferred) {
        if (ec)
            throw std::runtime_error("redis_helper: task_get: error during async write: " + ec.message());
        this_p->write_buffer_.consume(bytes_transferred);
        this_p->socket_.async_read(this_p->read_buffer_, [this_p, &handler](boost::system::error_code const& ec, result_t &&res) {
            if (ec)
                throw std::runtime_error("redis_helper: task_get: error during async read: " + ec.message());

            bredis::extracts::extraction_result_t extract = boost::apply_visitor(bredis::extractor<it_t>(), res.result);
            this_p->read_buffer_.consume(res.consumed);
            bool success;
            std::string str;
            try {
                str = std::move(boost::get<bredis::extracts::string_t>(extract).str);
                success = true;
            }
            catch (...) {
                success = false;
            }
            handler(success, std::move(str));
            this_p->exec_next();
        });
    });
}

void redis_helper::task_multi_set(std::vector<std::pair<std::string, std::string>> const& keys_values, std::function<void()> &handler)
{
    auto this_p = shared_from_this();
    bredis::command_container_t transaction;
    transaction.reserve(keys_values.size() + 2);
    transaction.emplace_back(bredis::single_command_t({ "MULTI" }));
    for (std::pair<std::string, std::string> const& key_value : keys_values)
        transaction.emplace_back(bredis::single_command_t({ "SET", key_value.first, key_value.second }));
    transaction.emplace_back(bredis::single_command_t({ "EXEC" }));

    socket_.async_write(write_buffer_, transaction, [this_p, &keys_values, &handler](boost::system::error_code const& ec, std::size_t bytes_transferred) {
        if (ec)
            throw std::runtime_error("redis_helper: task_multi_set: error during async write: " + ec.message());
        this_p->write_buffer_.consume(bytes_transferred);
        this_p->socket_.async_read(this_p->read_buffer_, [this_p, &keys_values, &handler](boost::system::error_code const& ec, result_t &&res) {
            if (ec)
                throw std::runtime_error("redis_helper: task_multi_set: error during async read: " + ec.message());

            // fixme

            //bredis::markers::array_holder_t<it_t> &results_array = boost::get<bredis::markers::array_holder_t<it_t>>(res.result);
            bredis::extracts::extraction_result_t extract = boost::apply_visitor(bredis::extractor<it_t>(), res.result);
            this_p->read_buffer_.consume(res.consumed);

            bredis::markers::array_holder_t<it_t> &results_array = boost::get<bredis::markers::array_holder_t<it_t>>(res.result);
            if (results_array.elements.size() != keys_values.size() + 2)
                throw std::runtime_error("redis_helper: task_multi_set: result array size is bad");

            //if (!boost::apply_visitor(bredis::marker_helpers::equality<it_t>("OK"), results_array.elements[0]))
                //throw std::runtime_error("");
            check_extract_str(boost::apply_visitor(bredis::extractor<it_t>(), results_array.elements[0]), "OK");


            int i;
            for (i = 1; i < keys_values.size() + 1; ++i)
                //if (!boost::apply_visitor(bredis::marker_helpers::equality<it_t>("QUEUED"), results_array.elements[i]))
                    //throw std::runtime_error("");
                check_extract_str(boost::apply_visitor(bredis::extractor<it_t>(), results_array.elements[i]), "QUEUED");

            bredis::extracts::extraction_result_t set_results_extract = boost::apply_visitor(bredis::extractor<it_t>(), results_array.elements[i]);
            bredis::extracts::array_holder_t *set_results_p;
            try {
                set_results_p = &boost::get<bredis::extracts::array_holder_t>(set_results_extract);
            }
            catch (...) {
                throw std::runtime_error("redis_helper: task_multi_set: set results is not an array");
            }

            if (set_results_p->elements.size() != keys_values.size())
                throw std::runtime_error("redis_helper: task_multi_set: set results array size is bad");

            for (bredis::extracts::extraction_result_t & res_element : set_results_p->elements)
                check_extract_str(res_element, "OK");

            handler();
            this_p->exec_next();
        }, keys_values.size() + 2);
    });
}

void redis_helper::exec_next()
{
    WEBGAME_LOCK(tasks_mutex_);

    tasks_.pop_front();

    if (tasks_.empty())
        return;

    tasks_.front()();
}

// HELPERS

void redis_helper::command_result_str(bredis::single_command_t const& cmd, std::string const& expected_res)
{
    socket_.write(cmd);
    result_t res = socket_.read(read_buffer_);
    bredis::extracts::extraction_result_t extract = boost::apply_visitor(bredis::extractor<it_t>(), res.result);
    read_buffer_.consume(res.consumed);
    check_extract_str(extract, expected_res);
}

void redis_helper::check_extract_str(bredis::extracts::extraction_result_t const& extract, std::string const& expected_res)
{
    std::string err_str;
    try {
        err_str = boost::get<bredis::extracts::error_t>(extract).str;
    }
    catch (...) {}

    if (!err_str.empty())
        throw std::runtime_error("redis_helper: " + err_str);

    std::string res_str;
    try {
        res_str = boost::get<bredis::extracts::string_t>(extract).str;
    }
    catch (std::exception const& e) {
        throw std::runtime_error("redis_helper: " + std::string(e.what()));
    }
    if (res_str != expected_res)
        throw std::runtime_error("redis_helper: result is not \"" + expected_res + "\"");
}

std::string redis_helper::command_to_string(bredis::single_command_t const& cmd)
{
    std::string str;
    for (boost::string_ref const& arg : cmd.arguments)
        str += arg.to_string();
    return str;
}

} // namespace webgame
