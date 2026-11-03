/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _logbf
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "libm.h"

/* Every float is a normal double, so the exponent comes out unchanged */
_Check_return_
float
__cdecl
_logbf(
    _In_ float x)
{
    /* Infinities come back unchanged, logb would make -inf positive */
    if ((asuint(x) & 0x7FFFFFFF) == 0x7F800000)
        return x;

    return (float)logb((double)x);
}
