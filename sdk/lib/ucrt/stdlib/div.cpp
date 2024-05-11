//
// div.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines div(), which performs a signed divide and returns the quotient and
// remainder.  No validation of the arguments is done.
//
#include <stdlib.h>


#ifdef  _MSC_VER
#pragma function(div)
#endif
extern "C" div_t __cdecl div(int const numerator, int const denominator)
{
    div_t result;

    result.quot = numerator / denominator;
    result.rem = numerator % denominator;

    return result;
}
