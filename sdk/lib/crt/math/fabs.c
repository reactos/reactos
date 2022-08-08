/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Portable implementation of fabs
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <math.h>

#ifdef _MSC_VER
#pragma function(fabs)
#endif

_Check_return_
double
__cdecl
fabs(
    _In_ double x)
{
    /* Load the value as uint64 */
    unsigned long long u64 = *(unsigned long long*)&x;

    /* Clear the sign bit */
    u64 &= ~(1ULL << 63);

    /* Check for NAN */
    if (u64 > 0x7FF0000000000000ull)
    {
#ifdef _M_IX86
        /* Set error bit */
        *(unsigned long long*)&x |= 0x0008000000000000ull;
#endif
        return x;
    }

    /* Convert back to double */
    return *(double*)&u64;
}
