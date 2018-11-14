#pragma once

#include <map>
#include <memory>

#include <nlohmann/json.hpp>

#include "behavior.hpp"
#include "config.hpp"
#include "entity.hpp"
#include "log.hpp"

namespace webgame {

class behavior;

class WEBGAME_API npc : public mobile_entity
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(npc);

public:
    typedef std::multimap<int, std::shared_ptr<behavior>> behaviors;

private:
    behaviors behaviors_;

public:
    npc() = default;
    npc(std::string const& type, vector const& pos, vector const& dir, double speed, double max_speed, behaviors && behaviors = behaviors());

public:
    virtual bool update(double d, env & env) override;
    virtual nlohmann::json save() const override;
    virtual void           load(nlohmann::json const& j) override;

private:
    void init_behaviors();
    void treatbehaviors(double d, env & env);

#ifdef WEBGAME_TESTS
public:
    virtual bool operator==(entity const& other) const override;
    virtual bool operator!=(entity const& other) const override;
#endif /* WEBGAME_TESTS */
};

} // namespace webgame
