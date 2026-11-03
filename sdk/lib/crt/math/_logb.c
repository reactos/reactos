/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _logb
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "libm.h"

_Check_return_
double
__cdecl
_logb(
    _In_ double x)
{
    /* Infinities come back unchanged, logb would make -inf positive */
    if ((asuint64(x) & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF0000000000000ULL)
        return x;

    return logb(x);
}
