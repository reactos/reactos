/***
*rand.c - random number generator
*
*       Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines rand(), srand() - random number generator
*
*******************************************************************************/

#include <cruntime.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef _MTRAND
#undef _MTRAND
#endif

#ifndef _MTRAND
static long holdrand = 1L;
#endif  /* _MTRAND */

/***
*void srand(seed) - seed the random number generator
*
*Purpose:
*       Seeds the random number generator with the int given.  Adapted from the
*       BASIC random number generator.
*
*Entry:
*       unsigned seed - seed to seed rand # generator with
*
*Exit:
*       None.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl srand (
        unsigned int seed
        )
{
#ifdef _MTRAND

        _getptd()->_holdrand = (unsigned long)seed;

#else  /* _MTRAND */
        holdrand = (long)seed;
#endif  /* _MTRAND */
}


/***
*int rand() - returns a random number
*
*Purpose:
*       returns a pseudo-random number 0 through 32767.
*
*Entry:
*       None.
*
*Exit:
*       Returns a pseudo-random number 0 through 32767.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl rand (
        void
        )
{
#ifdef _MTRAND

        _ptiddata ptd = _getptd();

        return( ((ptd->_holdrand = ptd->_holdrand * 214013L
            + 2531011L) >> 16) & 0x7fff );

#else  /* _MTRAND */
        return(((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
#endif  /* _MTRAND */
}
