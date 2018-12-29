#pragma once

#include <functional>
#include <memory>

#include "entities.hpp"

namespace webgame {

class player;

class WEBGAME_API persistence
{
public:
    typedef void(save_handler)();
    typedef void(get_player_handler)(bool, std::shared_ptr<player> const&);
    typedef void(check_player_handler)(bool, id_t);
    typedef void(add_player_handler)();

public:
    virtual ~persistence() {};

    virtual bool                    start() = 0;
    virtual void                    stop() = 0;
    virtual void                    async_save(entities const& ents, std::function<save_handler> &&handler) = 0;
    virtual entities                load_all_npes() = 0;
    virtual void					async_check_player(std::string const& name, std::function<check_player_handler> &&handler) = 0;
    virtual void                    async_get_player(id_t id, std::function<get_player_handler> &&handler) = 0;
    virtual void					async_add_player(std::string const& name, std::shared_ptr<player> const& player, std::function<add_player_handler> &&handler) = 0;
    virtual void                    remove_all() = 0;
};

} // namespace webgame
