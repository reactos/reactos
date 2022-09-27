/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x64 implementation of _clearfp
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <float.h>
#include <xmmintrin.h>

unsigned int __cdecl _clearfp(void)
{
    unsigned int retval;

    /* Get current status value */
    retval = _statusfp();

    /* Clear the exception mask */
    _mm_setcsr(_mm_getcsr() & ~_MM_EXCEPT_MASK);
   
    /* Return the previous state */
    return retval;
}
