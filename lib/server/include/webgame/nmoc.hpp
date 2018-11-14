#pragma once

#define WEBGAME_NON_MOVABLE_OR_COPYABLE(T)\
T(T const&) = delete;\
T(T &&) = delete;\
T &operator=(T const&) = delete;\
T &operator=(T &&) = delete;
