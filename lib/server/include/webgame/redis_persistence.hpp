#pragma once

#include <memory>
#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>

#include "nmoc.hpp"
#include "persistence.hpp"

namespace webgame {

class redis_helper;

class WEBGAME_API redis_persistence : public persistence, public std::enable_shared_from_this<redis_persistence>
{
private:
    std::shared_ptr<redis_helper> helper_;
    std::string host_;
    unsigned short port_;
    unsigned int index_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_context &io_context_;

private:
    WEBGAME_NON_MOVABLE_OR_COPYABLE(redis_persistence);

public:
    redis_persistence(boost::asio::io_context &io_context, std::string const& host, unsigned short port = 6379, unsigned int index = 0);

public:
    virtual bool                    start() override;
    virtual void                    stop() override;
    virtual void                    async_save(entities const& ents, std::function<save_handler> &&handler) override;
    virtual entities                load_all_npes() override;
    virtual void					async_check_player(std::string const& name, std::function<check_player_handler> &&handler) override;
    virtual void                    async_get_player(id_t id, std::function<get_player_handler> &&handler) override;
    virtual void					async_add_player(std::string const& name, std::shared_ptr<player> const& player, std::function<add_player_handler> &&handler) override;
    virtual void                    remove_all() override;
};

} // namespace webgame
