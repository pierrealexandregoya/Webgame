#pragma once

#include "misc/common.hpp"
#include "misc/nmoc.hpp"
#include "misc/vector.hpp"

class entity;
class env;

class behavior
{
protected:
    entity* self_;
    bool    resolved_;

private:
    NON_MOVABLE_OR_COPYABLE(behavior);

protected:
    behavior();

public:
    virtual ~behavior();

public:
    void set_self(entity *self);
    bool resolved() const;

public:
    virtual void update(duration delta, env &env) = 0;
};

class walkaround : public behavior
{
private:
    real t_;

private:
    NON_MOVABLE_OR_COPYABLE(walkaround);

public:
    walkaround();

    void update(duration delta, env &env) override;
};

class arealimit : public behavior
{
public:
    enum area_type
    {
        Square,
        Circle,
    };

private:
    area_type   area_type_;
    real        radius_;
    vector      center_;

private:
    NON_MOVABLE_OR_COPYABLE(arealimit);

public:
    arealimit(area_type type, real radius, vector const& center);

    void update(duration delta, env &env) override;
};

class attack_on_sight : public behavior
{
private:
    real  radius_;

private:
    NON_MOVABLE_OR_COPYABLE(attack_on_sight);

public:
    attack_on_sight(real radius);

    void update(duration delta, env &env) override;
};

class stop : public behavior
{
private:
    NON_MOVABLE_OR_COPYABLE(stop);

public:
    stop() = default;

    void update(duration delta, env &env) override;
};
