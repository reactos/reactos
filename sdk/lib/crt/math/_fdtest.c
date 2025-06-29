/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _fdtest.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <math.h>

_Check_return_ short __cdecl _fdclass(_In_ float _X);

#if defined(_MSC_VER) && (_MSC_VER >= 1922)
_Check_return_ short __cdecl _fdtest(_In_ float* _Px);
#pragma function(_fdtest)
#endif

_Check_return_
short
__cdecl
_fdtest(_In_ float* _Px)
{
    return _fdclass(*_Px);
}
