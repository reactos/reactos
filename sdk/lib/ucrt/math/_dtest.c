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

#if defined(_MSC_VER) && (_MSC_VER >= 1922)
#pragma function(_dtest)
#endif

_Check_return_
short
__cdecl
_dtest(_In_ double* _Px)
{
    return _dclass(*_Px);
}
