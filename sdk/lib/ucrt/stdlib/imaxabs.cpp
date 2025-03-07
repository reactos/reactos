//
// imaxabs.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines imaxabs(), which computes the absolute value of an intmax_t.
//
#include <inttypes.h>



extern "C" intmax_t __cdecl imaxabs(intmax_t const number)
{
    return number < 0 ? -number : number;
}



/*
 * Copyright (c) 1992-2010 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
V5.30:0009 */
