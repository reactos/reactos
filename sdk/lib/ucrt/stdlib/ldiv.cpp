//
// ldiv.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines ldiv(), which performs a signed divide and returns the quotient and
// remainder.  No validation of the arguments is done.
//
#include <stdlib.h>


#ifdef  _MSC_VER
#pragma function(ldiv)
#endif
extern "C" ldiv_t __cdecl ldiv(long const numerator, long const denominator)
{
    ldiv_t result;

    result.quot = numerator / denominator;
    result.rem = numerator % denominator;

    return result;
}
