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

#ifdef _MSC_VER
#pragma function(_fdtest)
#endif

_Check_return_
short
__cdecl
_fdtest(_In_ float* _Px)
{
    return _fdclass(*_Px);
}
