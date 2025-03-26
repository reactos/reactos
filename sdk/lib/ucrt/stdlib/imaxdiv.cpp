//
// imaxdiv.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines imaxdiv(), which performs a signed divide using intmax_t operands
// and returns the quotient and remainder.  No validation of the arguments is
// done.
//
#include <inttypes.h>



extern "C" imaxdiv_t __cdecl imaxdiv(intmax_t const numerator, intmax_t const denominator)
{
    imaxdiv_t result;

    result.quot = numerator / denominator;
    result.rem = numerator - denominator * result.quot;

    // Fix incorrect truncation:
    #pragma warning(push)
    #pragma warning(disable: 4127)
    static bool const fix_required = (-1 / 2) < 0;
    if (fix_required && result.quot < 0 && result.rem != 0)
    {
        result.quot += 1;
        result.rem -= denominator;
    }
    #pragma warning(pop)

    return result;
}



/*
 * Copyright (c) 1992-2010 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
V5.30:0009 */
