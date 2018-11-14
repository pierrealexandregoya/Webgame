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
    typedef void(load_player_handler)(std::shared_ptr<player> const&);

public:
    virtual ~persistence() {};

    virtual bool                    start() = 0;
    virtual void                    stop() = 0;
    virtual void                    async_save(entities const& ents, std::function<save_handler> &&handler) = 0;
    virtual entities                load_all_npes() = 0;
    virtual void                    async_load_player(std::string const& name, std::function<load_player_handler> &&handler) = 0;
    virtual void                    remove_all() = 0;
};

} // namespace webgame
