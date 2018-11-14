#pragma once

#include <boost/numeric/ublas/vector.hpp>

#include <nlohmann/json.hpp>

#include "common.hpp"
#include "config.hpp"

namespace ublas = boost::numeric::ublas;

namespace webgame {

/* Workaround for Boost UBLAS 1.67 and MSVC 2017 ambiguitie where
vector(U&&...) is prefered where vector(vector_expression<AE> const&)
should be chosen unless if constructor prototype is matching perfectly,
requiring unecessary cast code*/
using vec_impl = ublas::fixed_vector<double, 2>;
class WEBGAME_API vector : public vec_impl
{
public:
    vector() = default;
    vector(vector const& other) = default;
    vector(std::initializer_list<value_type> const& l);
    template<class T>
    vector(ublas::vector_expression<T> const& ve)
        : vec_impl(ve)
    {}
    vector(double x, double y);

    nlohmann::json save() const;
    void           load(nlohmann::json const& j);

    bool operator==(vector const& other) const;
    bool operator!=(vector const& other) const;

    double const& x() const;
    double const& y() const;

    double norm() const;
    vector normalized() const;
    void normalize();
};

WEBGAME_API std::ostream &operator<<(std::ostream &os, vector const&v);

} // namespace webgame
