#include "entity.hpp"

#include "random.hpp"
#include "save_load.hpp"

namespace webgame {

//-----------------------------------------------------------------------------
// ENTITY

entity::entity(std::string const& type)
    : id_(id_rand(gen))
    , type_(type)
{}

entity::~entity()
{}

nlohmann::json entity::save() const
{
    return {
        {"id", id_},
        {"type", type_}
    };
}

void entity::load(nlohmann::json const& j)
{
    if (!j.is_object() || !j.count("id") || !j["id"].is_number_unsigned()
        || !j.count("type") || !j["type"].is_string())
        throw std::runtime_error("entity: invalid JSON");

    id_ = j["id"];
    type_ = j["type"];
}

void entity::build_state_order(nlohmann::json &j) const
{
    j["id"] = id_;
    j["type"] = type_;
}

id_t const& entity::id() const
{
    return id_;
}

std::string const& entity::type() const
{
    return type_;
}

#ifdef WEBGAME_TESTS
bool entity::operator==(entity const& other) const
{
    return id_ == other.id_ && type_ == other.type_;
}

bool entity::operator!=(entity const& other) const
{
    return !(*this == other);
}
#endif /* WEBGAME_TESTS */

//-----------------------------------------------------------------------------
// LOCATED ENTITY

located_entity::located_entity(std::string const& type, vector const& pos)
    : entity(type)
    , pos_(pos)
{}

nlohmann::json located_entity::save() const
{
    return {
        {"entity", entity::save()},
        {"pos", pos_.save()} };
}

void located_entity::load(nlohmann::json const& j)
{
    if (!j.is_object() || !j.count("entity") || !j.count("pos"))
        throw std::runtime_error("located_entity: invalid JSON");

    entity::load(j["entity"]);
    pos_.load(j["pos"]);
}

void located_entity::build_state_order(nlohmann::json &j) const
{
    entity::build_state_order(j);
    j["pos"] = pos_.save();
}

void located_entity::set_pos(vector const& pos)
{
    pos_ = pos;
}

vector const& located_entity::pos() const
{
    return pos_;
}

#ifdef WEBGAME_TESTS
bool located_entity::operator==(entity const& other) const
{
    if (!entity::operator==(other))
        return false;

    located_entity const& true_other = static_cast<decltype(true_other)>(other);

    if (pos_ != true_other.pos_)
        return false;

    return true;
}

bool located_entity::operator!=(entity const& other) const
{
    return !(*this == other);
}
#endif /* WEBGAME_TESTS */

//-----------------------------------------------------------------------------
// MOBILE_ENTITY

mobile_entity::mobile_entity(std::string const& type, vector const& pos, vector const& dir, double speed, double max_speed)
    : located_entity(type, pos)
    , dir_(dir)
    , speed_(speed)
    , max_speed_(max_speed)
{}

bool mobile_entity::update(double d, env & env)
{
    vector prev_pos = pos_;
    vector prev_dir = dir_;

    if (dir_.norm() > std::numeric_limits<double>::epsilon() && speed_ > std::numeric_limits<double>::epsilon())
    {
        dir_.normalize();
        pos_ = pos_ + dir_ * speed_ * d;
        assert(!std::isnan(dir_[0]));
        assert(!std::isnan(dir_[1]));
        assert(!std::isnan(pos_[0]));
        assert(!std::isnan(pos_[1]));
        assert(!std::isnan(speed_));
    }
    return prev_pos != pos_ || prev_dir != dir_;
}

nlohmann::json mobile_entity::save() const
{
    return {
        {"located_entity", located_entity::save()},
        {"dir", dir_.save()},
        {"speed", speed_},
        {"max_speed", max_speed_}
    };
}

void mobile_entity::load(nlohmann::json const& j)
{
    if (!j.is_object()
        || !j.count("located_entity") || !j.count("dir") || !j.count("speed")
        || !j["speed"].is_number_float() || !j.count("max_speed")
        || !j["max_speed"].is_number_float())
        throw std::runtime_error("mobile_entity: invalid JSON");

    located_entity::load(j["located_entity"]);
    dir_.load(j["dir"]);
    speed_ = j["speed"].get<double>();
    max_speed_ = j["max_speed"].get<double>();
}

void mobile_entity::build_state_order(nlohmann::json &j) const
{
    located_entity::build_state_order(j);
    j["dir"] = dir_.save();
    j["speed"] = speed_;
}

void mobile_entity::set_dir(vector const& dir)
{
    dir_ = dir;
}

void mobile_entity::set_speed(double speed)
{
    speed_ = std::max(std::min(speed, max_speed_), 0.);
}

void mobile_entity::set_max_speed(double max_speed)
{
    max_speed_ = max_speed;
}

vector const& mobile_entity::dir() const
{
    return dir_;
}

double const& mobile_entity::speed() const
{
    return speed_;
}

double const& mobile_entity::max_speed() const
{
    return max_speed_;
}

#ifdef WEBGAME_TESTS
bool mobile_entity::operator==(entity const& other) const
{
    if (!located_entity::operator==(other))
        return false;

    mobile_entity const& true_other = static_cast<decltype(true_other)>(other);

    if (dir_ != true_other.dir_
        || speed_ != true_other.speed_
        || max_speed_ != true_other.max_speed_)
        return false;

    return true;
}

bool mobile_entity::operator!=(entity const& other) const
{
    return !(*this == other);
}
#endif /* WEBGAME_TESTS */

} // namespace webgame
