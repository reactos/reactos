/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Portable implementation of ceil
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _USE_MATH_DEFINES
#include <math.h>
#include <limits.h>

#ifdef _MSC_VER
#pragma function(ceil)
#endif

double
__cdecl
ceil(double x)
{
    /* Load the value as uint64 */
    unsigned long long u64 = *(unsigned long long*)&x;

    /* Check for NAN */
    if ((u64 & ~(1ULL << 63)) > 0x7FF0000000000000ull)
    {
        /* Set error bit */
        u64 |= 0x0008000000000000ull;
        return *(double*)&u64;
    }

    /* Check if x is positive */
    if ((u64 & (1ULL << 63)) == 0)
    {
        /* Check if it fits into an int64 */
        if (x < (double)_I64_MAX)
        {
            /* Cast to int64 to truncate towards 0. If this matches the
               input, return it as is, otherwise add 1 */
            double y = (double)(long long)x;
            return (x > y) ? y + 1 : y;
        }
        else
        {
            /* The exponent is larger than the fraction bits.
               This means the number is already an integer. */
            return x;
        }
    }
    else
    {
        /* Check if it fits into an int64 */
        if (x > (double)_I64_MIN)
        {
            /* Cast to int64 to truncate towards 0. */
            x = (double)(long long)x;
            return (x == 0.) ? -0.0 : x;
        }
        else
        {
            /* The exponent is larger than the fraction bits.
               This means the number is already an integer. */
            return x;
        }
    }
}
