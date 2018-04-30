#pragma once

#include <memory>
#include <map>

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
//template<class T, class args...>
//using MP = std::function<P<T>(args...)>(&std::make_shared<T>, args...);

class Behavior;
class Entity;
class WsConn;
class Env;
class Walk;
class AreaType;

typedef boost::numeric::ublas::vector<float> VecType;
typedef std::array<float, 3> Array2;
typedef std::map<float, P<Behavior>> Behaviors;
typedef std::map<int, P<Entity>> Entities;
typedef std::map<int, P<WsConn>> Connections;

// private T(... ?
#define NON_MOVABLE_OR_COPYABLE(T)\
private:\
T(T const&);\
T(T &&);\
T &operator=(T const&);\
T &operator=(T &&);
