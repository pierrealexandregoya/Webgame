#pragma once

#include "common.hpp"
#include "nmoc.hpp"
#include "serialization.hpp"
#include "vector.hpp"

class entity;
class env;

class behavior
{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        //SERIALIZATION_LOG("behavior");
        ar & resolved_;
    }

protected:
    entity* self_;
    bool    resolved_;

private:
    NON_MOVABLE_OR_COPYABLE(behavior);

#ifndef NDEBUG
public:
    virtual bool operator==(behavior const& other);
    virtual bool operator!=(behavior const& other);
#endif /* !NDEBUG */

protected:
    behavior();

public:
    virtual ~behavior();

public:
    void set_self(entity *self);
    bool const& resolved() const;

public:
    virtual void update(duration delta, env &env) = 0;
};

class walkaround : public behavior
{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        //SERIALIZATION_LOG("walkaround");
        ar & boost::serialization::base_object<behavior>(*this);
        ar & t_;
    }

private:
    real t_;

private:
    NON_MOVABLE_OR_COPYABLE(walkaround);

#ifndef NDEBUG
public:
    virtual bool operator==(behavior const& o);
    virtual bool operator!=(behavior const& other);
#endif /* !NDEBUG */

public:
    walkaround();

    void update(duration delta, env &env) override;
};

class arealimit : public behavior
{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        //SERIALIZATION_LOG("arealimit");
        ar & boost::serialization::base_object<behavior>(*this);
        ar & area_type_;
        ar & radius_;
        ar & center_;
    }

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
    arealimit() = default;

#ifndef NDEBUG
public:
    virtual bool operator==(behavior const& o);
    virtual bool operator!=(behavior const& other);
#endif /* !NDEBUG */

public:
    arealimit(area_type type, real radius, vector const& center);

    void update(duration delta, env &env) override;
};

class attack_on_sight : public behavior
{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        //SERIALIZATION_LOG("attack_on_sight");
        ar & boost::serialization::base_object<behavior>(*this);
        ar & radius_;
    }

private:
    real  radius_;

private:
    NON_MOVABLE_OR_COPYABLE(attack_on_sight);
    attack_on_sight() = default;

#ifndef NDEBUG
public:
    virtual bool operator==(behavior const& o);
    virtual bool operator!=(behavior const& other);
#endif /* !NDEBUG */

public:
    attack_on_sight(real radius);

    void update(duration delta, env &env) override;
};

class stop : public behavior
{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        //SERIALIZATION_LOG("stop");
        ar & boost::serialization::base_object<behavior>(*this);
    }

private:
    NON_MOVABLE_OR_COPYABLE(stop);

public:
    stop() = default;

    void update(duration delta, env &env) override;
};
