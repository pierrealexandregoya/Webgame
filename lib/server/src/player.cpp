#include "player.hpp"

#include <boost/geometry/arithmetic/dot_product.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include "save_load.hpp"

namespace webgame {

player::player()
    : mobile_entity("player", { 0, 0 }, { 0, -1 }, 0, 1)
    , moving_to_(false)
{}

bool player::update(double d, env & env)
{
    bool has_changed = mobile_entity::update(d, env);
    if (moving_to_)
    {
        if (speed_ != 0)
        {
            vector to_vec = target_pos_ - pos_;
            boost::geometry::model::d2::point_xy<double> geo_to_vec(to_vec.x(), to_vec.y());
            boost::geometry::model::d2::point_xy<double> geo_dir(dir_.x(), dir_.y());
            if (pos_ == target_pos_ || boost::geometry::dot_product(geo_to_vec, geo_dir) < 0)
            {
                speed_ = 0;
                pos_ = target_pos_;
                moving_to_ = false;
                return true;
            }
        }
        else
            moving_to_ = false;
    }
    return has_changed;
}

nlohmann::json player::save() const
{
    return {
        {"mobile_entity", mobile_entity::save()},
        {"target_pos", target_pos_.save()},
        {"moving_to", moving_to_},
        {"type", "player"}
    };
}

void player::load(nlohmann::json const& j)
{
    if (!j.is_object() || !j.count("mobile_entity") || !j.count("target_pos")
        || !j.count("moving_to") || !j["moving_to"].is_boolean())
        throw std::runtime_error("player: invalid JSON");

    mobile_entity::load(j["mobile_entity"]);
    target_pos_.load(j["target_pos"]);
    moving_to_ = j["moving_to"].get<bool>();
}

void player::move_to(vector const& target_pos)
{
    if (speed_ == 0)
        return;
    target_pos_ = target_pos;
    dir_ = target_pos_ - pos_;
    moving_to_ = true;
}

void player::stop()
{
    moving_to_ = false;
}

bool player::is_moving_to() const
{
    return moving_to_;
}

void player::set_conn(std::shared_ptr<player_conn> const& conn)
{
    player_conn_ = conn;
}

std::shared_ptr<player_conn> player::conn()
{
    return player_conn_;
}

#ifdef WEBGAME_TESTS
bool player::operator==(entity const& other) const
{
    if (!mobile_entity::operator==(other))
        return false;

    player const& true_other = static_cast<decltype(true_other)>(other);

    if (target_pos_ != true_other.target_pos_
        || moving_to_ != true_other.moving_to_)
        return false;

    return true;
}

bool player::operator!=(entity const& other) const
{
    return !(*this == other);
}
#endif /* WEBGAME_TESTS */

WEBGAME_REGISTER(entity, player);

} // namespace webgame
