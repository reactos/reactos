//
// lldiv.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines lldiv(), which performs a signed divide and returns the quotient and
// remainder.  No validation of the arguments is done.
//
#include <stdlib.h>


#ifdef  _MSC_VER
#pragma function(lldiv)
#endif
extern "C" lldiv_t __cdecl lldiv(long long const numerator, long long const denominator)
{
    lldiv_t result;

    result.quot = numerator / denominator;
    result.rem = numerator % denominator;

    return result;
}
