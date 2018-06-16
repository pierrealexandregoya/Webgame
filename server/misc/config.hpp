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
