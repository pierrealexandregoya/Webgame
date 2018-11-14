#pragma once

#include "entity.hpp"

namespace webgame {

class WEBGAME_API stationnary_entity : public located_entity
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(stationnary_entity);

public:
    stationnary_entity() = default;
    stationnary_entity(std::string const& type, vector const& pos);

public:
    virtual bool           update(double d, env & env) override;
    virtual nlohmann::json save() const override;
    virtual void           load(nlohmann::json const& j) override;
};

} // namespace webgame
