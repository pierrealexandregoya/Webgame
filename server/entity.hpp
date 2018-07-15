#pragma once

#include <memory>

#include <boost/serialization/map.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/split_member.hpp>

#include "behavior.hpp"
#include "common.hpp"
#include "containers.hpp"
#include "nmoc.hpp"
#include "serialization.hpp"
#include "vector.hpp"

class env;

class entity : public std::enable_shared_from_this<entity>
{
private:
    // For access to private default constructor
    template<class T>
    friend T deserialize(std::string const & s);

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        //SERIALIZATION_LOG("entity");
        ar & id_;
        ar & pos_;
        ar & dir_;
        ar & type_;
        ar & behaviors_;
        ar & speed_;
        ar & max_speed_;

        boost::serialization::split_member(ar, *this, version);
    }

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {}

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        init_behaviors();
    }

private:
    id_t        id_;
    vector      pos_;
    vector      dir_;
    std::string type_;
    behaviors_t behaviors_;
    real        speed_;
    real        max_speed_;

#ifndef NDEBUG
public:
    entity(entity const&) = default;
    bool operator==(entity const& other);
    bool operator!=(entity const& other);
#else
private:
    entity(entity const&) = delete;
#endif /* !NDEBUG */

private:
    entity &operator=(entity const&) = delete;
    entity &operator=(entity &&) = delete;

    entity() = default;

public:
    entity(vector const& pos, vector const& dir, real speed, real max_speed, std::string const& type, behaviors_t && behaviors = behaviors_t());
    entity(entity && other);

private:
    void init_behaviors();

public:
    bool                update(real d, env & env);
    void                treatbehaviors(real d, env & env);
    void                set_dir(vector const& vec);
    void                set_speed(real speed);
    id_t const&         id() const;
    vector const&       pos() const;
    vector const&       dir() const;
    std::string const&  type() const;
    real const&         speed() const;
    real const&         max_speed() const;
};
