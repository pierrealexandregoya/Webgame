#pragma once

#include "config.hpp"

#if _HAS_CXX17==1
    #include <any>
    using std::any;
    using std::any_cast;
    using std::bad_any_cast;
#else
    #include <boost/any.hpp>
    using boost::any;
    using boost::any_cast;
    using boost::bad_any_cast;
#endif /* _HAS_CXX17==1 */
