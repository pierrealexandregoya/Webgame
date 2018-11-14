#pragma once

#define TEST_LOG(to_log) WEBGAME_LOG(std::string(webgame::title_max_size / 2 - 2, '-') + "TEST" + std::string(webgame::title_max_size / 2 - 2, '-'), to_log)

template<class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
almost_equal(T x, T y, int ulp = 1)
{
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    return std::abs(x - y) <= std::numeric_limits<T>::epsilon() * std::abs(x + y) * ulp
        // unless the result is subnormal
        || std::abs(x - y) < std::numeric_limits<T>::min();
}
