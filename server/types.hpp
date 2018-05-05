#pragma once

#include <memory>
#include <map>
#include <array>

#include <boost/numeric/ublas/vector.hpp>

// Pointer type
template<class T>
using P = std::shared_ptr<T>;

// Make pointer
template<class T, class ...Us>
P<T> MP(Us ...args)
{
    return std::make_shared<T>(args...);
}

class Behavior;
class Entity;
class WsConn;
class Env;
class Walk;
class AreaLimit;

using Real = float;

/* Workaround for Boost UBLAS 1.67 and MSVC 2017 ambiguitie where
vector(U&&...) is prefered where vector(vector_expression<AE> const&)
should be chosen unless if constructor prototype is matching perfectly,
requiring unecessary cast code*/
using VecImpl = boost::numeric::ublas::fixed_vector<Real, 2>;
class Vector : public VecImpl
{
public:
    Vector() = default;

    Vector(std::initializer_list<value_type> const& l)
    {
        assert(l.size() == 2);
        auto it = l.begin();
        (*this)[0] = *it;
        ++it;
        (*this)[1] = *it;
    }

    template<class T>
    Vector(boost::numeric::ublas::vector_expression<T> const& ve)
        : VecImpl(ve)
    {}
};

typedef std::map<float, P<Behavior>> Behaviors;
typedef std::map<int, P<Entity>> Entities;
typedef std::map<int, P<WsConn>> Connections;

#define NON_MOVABLE_OR_COPYABLE(T)\
private:\
T(T const&);\
T(T &&);\
T &operator=(T const&);\
T &operator=(T &&);
