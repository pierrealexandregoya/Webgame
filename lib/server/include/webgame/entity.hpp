#pragma once

#include <nlohmann/json.hpp>

#include "common.hpp"
#include "config.hpp"
#include "log.hpp"
#include "nmoc.hpp"
#include "vector.hpp"

namespace webgame {

class env;

//-----------------------------------------------------------------------------
// ENTITY

class WEBGAME_API entity
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(entity);

protected:
    id_t        id_;
    std::string type_;

protected:
    entity() = default;
    entity(std::string const& type);
    virtual ~entity();

public:
    virtual bool           update(double d, env & env) = 0;
    virtual nlohmann::json save() const;
    virtual void           load(nlohmann::json const& j);
    virtual void           build_state_order(nlohmann::json &j) const;

    id_t const&        id() const;
    std::string const& type() const;

#ifdef WEBGAME_TESTS
public:
    virtual bool operator==(entity const& other) const;
    virtual bool operator!=(entity const& other) const;
#endif /* WEBGAME_TESTS */
};

//-----------------------------------------------------------------------------
// LOCATED ENTITY

class WEBGAME_API located_entity : public entity
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(located_entity);

protected:
    vector pos_;

protected:
    located_entity() = default;
    located_entity(std::string const& type, vector const& pos);

public:
    virtual nlohmann::json save() const override;
    virtual void           load(nlohmann::json const& j) override;
    virtual void           build_state_order(nlohmann::json &j) const override;

    void          set_pos(vector const& pos);
    vector const& pos() const;

#ifdef WEBGAME_TESTS
public:
    virtual bool operator==(entity const& other) const override;
    virtual bool operator!=(entity const& other) const override;
#endif /* WEBGAME_TESTS */
};

//-----------------------------------------------------------------------------
// MOBILE ENTITY

class WEBGAME_API mobile_entity : public located_entity
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(mobile_entity);

protected:
    vector dir_;
    double speed_;
    double max_speed_;

protected:
    mobile_entity() = default;
    mobile_entity(std::string const& type, vector const& pos, vector const& dir, double speed, double max_speed);

public:
    virtual bool           update(double d, env & env) override;
    virtual nlohmann::json save() const override;
    virtual void           load(nlohmann::json const& j) override;
    virtual void           build_state_order(nlohmann::json &j) const override;

    void set_dir(vector const& vec);
    void set_speed(double speed);
    void set_max_speed(double speed);

    vector const& dir() const;
    double const& speed() const;
    double const& max_speed() const;

#ifdef WEBGAME_TESTS
public:
    virtual bool operator==(entity const& other) const override;
    virtual bool operator!=(entity const& other) const override;
#endif /* WEBGAME_TESTS */
};

} // namespace webgame
