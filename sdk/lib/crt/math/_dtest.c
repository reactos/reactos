/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _dtest.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <math.h>

_Check_return_ short __cdecl _dclass(_In_ double _X);

#if defined(_MSC_VER) && (_MSC_VER >= 1922)
_Check_return_ short __cdecl _dtest(_In_ double* _Px);
#pragma function(_dtest)
#endif

_Check_return_
short
__cdecl
_dtest(_In_ double* _Px)
{
    return _dclass(*_Px);
}
