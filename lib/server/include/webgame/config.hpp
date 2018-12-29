#pragma once

// Test c++17
#ifndef _HAS_CXX17
# if defined(_MSVC_LANG) && !(defined(__EDG__) && defined(__clang__))  // TRANSITION, VSO#273681
#  if _MSVC_LANG > 201402
#   define _HAS_CXX17  1
#  else /* _MSVC_LANG > 201402 */
#   define _HAS_CXX17  0
#  endif /* _MSVC_LANG > 201402 */
# else /* _MSVC_LANG etc. */
#  if __cplusplus > 201402
#   define _HAS_CXX17  1
#  else /* __cplusplus > 201402 */
#   define _HAS_CXX17  0
#  endif /* __cplusplus > 201402 */
# endif /* _MSVC_LANG etc. */
#endif /* !_HAS_CXX17 */

// Test C++20 (fixme: proper macro)
#ifndef _HAS_CXX20
# if __cplusplus > 201703L
#  define _HAS_CXX20 1
# else /* __cplusplus > 201703L */
#  define _HAS_CXX20 0
# endif /* __cplusplus > 201703L */
#endif /* !HAS_CXX20 */

#if defined(_WIN32)
# define WEBGAME_SYSTEM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
# include "TargetConditionals.h"
# if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#  define WEBGAME_SYSTEM_IOS
# elif TARGET_OS_MAC
#  define WEBGAME_SYSTEM_MACOS
# else
#  error This Apple operating system is not supported by Webgame library
# endif
#elif defined(__unix__)
# if defined(__ANDROID__)
#  define WEBGAME_SYSTEM_ANDROID
# elif defined(__linux__)
#  define WEBGAME_SYSTEM_LINUX
# elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#  define WEBGAME_SYSTEM_FREEBSD
# else
#  error This UNIX operating system is not supported by Webgame library
# endif
#else
# error This operating system is not supported by Webgame library
# endif

#if !defined(WEBGAME_STATIC)
# if defined(WEBGAME_SYSTEM_WINDOWS)
#  define WEBGAME_SYMBOL_EXPORT __declspec(dllexport)
#  define WEBGAME_SYMBOL_IMPORT __declspec(dllimport)
#  ifdef _MSC_VER
#   pragma warning(disable: 4251)
#  endif
# else // Linux, FreeBSD, Mac OS X
#  if __GNUC__ >= 4
#   define WEBGAME_SYMBOL_EXPORT __attribute__ ((__visibility__ ("default")))
#   define WEBGAME_SYMBOL_IMPORT __attribute__ ((__visibility__ ("default")))
#  else
#   define WEBGAME_SYMBOL_EXPORT
#   define WEBGAME_SYMBOL_IMPORT
#  endif
# endif
#else
# define WEBGAME_SYMBOL_EXPORT
# define WEBGAME_SYMBOL_IMPORT
#endif

#if defined(WEBGAME_BUILDING_THE_LIB)
# define WEBGAME_API WEBGAME_SYMBOL_EXPORT
#else
# define WEBGAME_API WEBGAME_SYMBOL_IMPORT
#endif
