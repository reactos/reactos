//
// _fdtest.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of _fdtest.
//
// SPDX-License-Identifier: MIT
//

#include <math.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1922)
#pragma function(_fdtest)
#endif

_Check_return_
short
__cdecl
_fdtest(_In_ float* _Px)
{
    return _fdclass(*_Px);
}
