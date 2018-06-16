#pragma once

#include <boost/numeric/ublas/vector.hpp>

#include "common.hpp"

namespace ublas = boost::numeric::ublas;

/* Workaround for Boost UBLAS 1.67 and MSVC 2017 ambiguitie where
vector(U&&...) is prefered where vector(vector_expression<AE> const&)
should be chosen unless if constructor prototype is matching perfectly,
requiring unecessary cast code*/
using vec_impl = ublas::fixed_vector<real, 2>;
class vector : public vec_impl
{
public:
    vector() = default;
    vector(vector const& other) = default;
    vector(std::initializer_list<value_type> const& l);
    template<class T>
    vector(ublas::vector_expression<T> const& ve)
        : vec_impl(ve)
    {}

    real const& x() const;
    real const& y() const;
};

std::ostream &operator<<(std::ostream &os, vec_impl const&v);
