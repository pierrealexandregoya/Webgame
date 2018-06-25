#pragma once

#include <memory>

#include "misc/common.hpp"
#include "misc/containers.hpp"
#include "misc/nmoc.hpp"
#include "misc/vector.hpp"

class env;

class entity : public std::enable_shared_from_this<entity>
{
private:
    id_t        id_;
    vector      pos_;
    vector      dir_;
    std::string type_;
    behaviors   behaviors_;
    real        speed_;
    real        max_speed_;

private:
    NON_MOVABLE_OR_COPYABLE(entity);

public:
    entity(vector const& pos, vector const& dir, real speed, real max_speed, std::string const& type, behaviors && behaviors = behaviors());

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
