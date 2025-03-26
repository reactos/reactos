//
// rand.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines rand(), which generates psuedorandom numbers.
//
#include <corecrt_internal.h>
#include <stdlib.h>



// Seeds the random number generator with the provided integer.
extern "C" void __cdecl srand(unsigned int const seed)
{
    __acrt_getptd()->_rand_state = seed;
}



// Returns a pseudorandom number in the range [0,32767].
extern "C" int __cdecl rand()
{
    __acrt_ptd* const ptd = __acrt_getptd();

    ptd->_rand_state = ptd->_rand_state * 214013 + 2531011;
    return (ptd->_rand_state >> 16) & RAND_MAX;
}
