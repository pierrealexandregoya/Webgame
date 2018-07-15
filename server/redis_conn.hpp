#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <bredis/Command.hpp>
#include <bredis/Connection.hpp>
#include <bredis/Extract.hpp>

#include "persistence.hpp"

using buffer_t = boost::asio::streambuf;
using it_t = typename bredis::to_iterator<buffer_t>::iterator_t;
using policy_t = bredis::parsing_policy::keep_result;
using result_t = bredis::parse_result_mapper_t<it_t, policy_t>;

class redis_conn : public persistence
{
private:
    bredis::Connection<boost::asio::ip::tcp::socket>    socket_;
    boost::asio::streambuf                              read_buffer_;

public:
    redis_conn(boost::asio::ip::tcp::socket &&socket);

public:
    virtual void                    save(entities const& ents);
    virtual entities                load_all_npes();
    virtual std::shared_ptr<entity> load_player(std::string const& name);
    virtual void                    remove_all();

private:
    std::vector<bredis::extracts::extraction_result_t>  redis_send_transaction(bredis::command_container_t const& cmds);
};
