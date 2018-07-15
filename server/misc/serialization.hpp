#pragma once

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

template<class T>
T deserialize(std::string const & s)
{
    std::istringstream ss(s);

    boost::archive::text_iarchive ia(ss);

    T t;

    ia >> t;

    return t;
}

template<class T>
std::string serialize(T const& t)
{
    std::ostringstream os;
    boost::archive::text_oarchive oa(os);

    oa << t;
    return os.str();
}

template<class Archive>
using is_load = std::is_base_of<boost::archive::detail::interface_iarchive<Archive>, Archive>;

#include "log.hpp"

#define SERIALIZATION_LOG(what) LOG((is_load<Archive>() ? "LOAD" : "SAVE"), what)
