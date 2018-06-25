#include "vector.hpp"

vector::vector(std::initializer_list<value_type> const& l)
{
    assert(l.size() == 2);
    auto it = l.begin();
    (*this)[0] = *it;
    ++it;
    (*this)[1] = *it;
}

bool vector::operator==(vector const& other)
{
    return (*this)[0] == other[0] && (*this)[1] == other[1];
}

bool vector::operator!=(vector const& other)
{
    return !(*this == other);
}

real const& vector::x() const
{
    return (*this)[0];
}

real const& vector::y() const
{
    return (*this)[1];
}

std::ostream &operator<<(std::ostream &os, vec_impl const&v)
{
    os << "(";
    for (auto i = 0; i < v.size(); ++i)
    {
        os << v[i];
        if (i + 1 == v.size())
            os << ",";
    }
    os << ")";
    return os;
}
