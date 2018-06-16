#pragma once

#include "config.hpp"

#if _HAS_CXX17==1
#include <filesystem>
using namespace std::filesystem;
#else
#include <boost/filesystem.hpp>
using namespace boost::filesystem;
#endif /* _HAS_CXX17==1 */
