#pragma once

#include <nlohmann/json.hpp>

#include "common.hpp"
#include "config.hpp"
#include "log.hpp"
#include "nmoc.hpp"
#include "vector.hpp"

namespace webgame {

class npc;
class env;

//-----------------------------------------------------------------------------
// BEHAVIOR

class WEBGAME_API behavior
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(behavior);

protected:
    npc* self_;
    bool resolved_;

protected:
    behavior();

public:
    virtual ~behavior();

public:
    virtual void update(double delta, env &env) = 0;

    virtual nlohmann::json save() const;
    virtual void           load(nlohmann::json const& j);

    void        set_self(npc *self);
    bool const& resolved() const;

#ifdef WEBGAME_TESTS
public:
    virtual bool operator==(behavior const& other) const;
    virtual bool operator!=(behavior const& other) const;
#endif /* WEBGAME_TESTS */
};

//-----------------------------------------------------------------------------
// WALKAROUND

class WEBGAME_API walkaround : public behavior
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(walkaround);

private:
    double t_;

public:
    walkaround();

public:
    virtual void update(double delta, env &env) override;
    virtual nlohmann::json save() const override;
    virtual void           load(nlohmann::json const& j) override;

#ifdef WEBGAME_TESTS
public:
    virtual bool operator==(behavior const& o) const override;
    virtual bool operator!=(behavior const& other) const override;
#endif /* WEBGAME_TESTS */
};

//-----------------------------------------------------------------------------
// AREALIMIT

class WEBGAME_API arealimit : public behavior
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(arealimit);

public:
    enum area_type
    {
        square,
        circle,
    };

private:
    area_type area_type_;
    double    radius_;
    vector    center_;

public:
    arealimit() = default;
    arealimit(area_type type, double radius, vector const& center);

public:
    virtual void           update(double delta, env &env) override;
    virtual nlohmann::json save() const override;
    virtual void           load(nlohmann::json const& j) override;

#ifdef WEBGAME_TESTS
public:
    virtual bool operator==(behavior const& o) const override;
    virtual bool operator!=(behavior const& other) const override;
#endif /* WEBGAME_TESTS */
};

//-----------------------------------------------------------------------------
// ATTACK ON SIGHT

class WEBGAME_API attack_on_sight : public behavior
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(attack_on_sight);

private:
    double radius_;

public:
    attack_on_sight() = default;
    attack_on_sight(double radius);

public:
    virtual void           update(double delta, env &env) override;
    virtual nlohmann::json save() const override;
    virtual void           load(nlohmann::json const& j) override;

#ifdef WEBGAME_TESTS
public:
    virtual bool operator==(behavior const& o) const override;
    virtual bool operator!=(behavior const& other) const override;
#endif /* WEBGAME_TESTS */
};

//-----------------------------------------------------------------------------
// STOP

class WEBGAME_API stop : public behavior
{
    WEBGAME_NON_MOVABLE_OR_COPYABLE(stop);

public:
    stop() = default;

public:
    virtual void           update(double delta, env &env) override;
    virtual nlohmann::json save() const override;
    virtual void           load(nlohmann::json const& j) override;
};

} // namespace webgame
