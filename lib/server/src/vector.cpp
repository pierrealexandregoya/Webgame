#include "vector.hpp"

namespace webgame {

vector::vector(std::initializer_list<value_type> const& l)
{
    assert(l.size() == 2);
    auto it = l.begin();
    (*this)[0] = *it;
    ++it;
    (*this)[1] = *it;
}

vector::vector(double x, double y)
{
    (*this)[0] = x;
    (*this)[1] = y;
}

nlohmann::json vector::save() const
{
    return { {"x", x()}, {"y", y()} };
}

void vector::load(nlohmann::json const& j)
{
    if (!j.is_object()
        || !j.count("x")
        || !j["x"].is_number_float()
        || !j.count("y")
        || !j["y"].is_number_float())
        throw std::runtime_error("vector: invalid JSON");

    (*this)[0] = j["x"].get<double>();
    (*this)[1] = j["y"].get<double>();
}

bool vector::operator==(vector const& other) const
{
    return (*this)[0] == other[0] && (*this)[1] == other[1];
}

bool vector::operator!=(vector const& other) const
{
    return !(*this == other);
}

double const& vector::x() const
{
    return (*this)[0];
}

double const& vector::y() const
{
    return (*this)[1];
}

double vector::norm() const
{
    return ublas::norm_2(*this);
}

vector vector::normalized() const
{
    return *this / norm();
}

void vector::normalize()
{
    *this /= norm();
}

std::ostream &operator<<(std::ostream &os, vector const&v)
{
    os << "(" << v[0] << "," << v[1] << ")";
    return os;
}

} // namespace webgame
