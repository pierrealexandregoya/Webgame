#pragma once

#include "config.hpp"
#include "entity.hpp"
#include "log.hpp"

namespace webgame {

class player_conn;

class WEBGAME_API player : public mobile_entity
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(player);

private:
    player_conn *player_conn_;

public:
    player() = default;

public:
    player(std::string const& player_type);

public:
    virtual void interpretAction(nlohmann::json const& j) = 0;

    void         set_conn(player_conn *conn);
    player_conn *conn();
};

class WEBGAME_API upview_player : public player
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(upview_player);

private:

    vector target_pos_;
    bool moving_to_;

public:
    upview_player();

public:
    virtual bool           update(double d, env & env) override;
    virtual nlohmann::json save() const override;
    virtual void           load(nlohmann::json const& j) override;
    virtual void           interpretAction(nlohmann::json const& j) override;

    void move_to(vector const& target_pos);
    void stop();
    bool is_moving_to() const;



#ifdef WEBGAME_TESTS
public:
    virtual bool operator==(entity const& other) const override;
    virtual bool operator!=(entity const& other) const override;
#endif /* WEBGAME_TESTS */
};

} // namespace webgame

