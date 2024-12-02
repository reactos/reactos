//
// _dtest.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of _dtest.
//
// SPDX-License-Identifier: MIT
//

#include <math.h>

#ifdef _MSC_VER
#pragma function(_dtest)
#endif

_Check_return_
short
__cdecl
_dtest(_In_ double* _Px)
{
    return _dclass(*_Px);
}
