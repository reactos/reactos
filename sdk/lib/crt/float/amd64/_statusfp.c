/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x64 implementation of _statusfp
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <float.h>
#include <xmmintrin.h>

unsigned int __cdecl _statusfp(void)
{
    unsigned int mxcsr, status = 0;

    /* Get MXCSR */
    mxcsr = _mm_getcsr();

    /* Convert to abstract status flags */
    if (mxcsr & _MM_EXCEPT_INVALID)
        status |= _SW_INVALID;
    if (mxcsr & _MM_EXCEPT_DENORM)
        status |= _SW_DENORMAL;
    if (mxcsr & _MM_EXCEPT_DIV_ZERO)
        status |= _SW_ZERODIVIDE;
    if (mxcsr & _MM_EXCEPT_OVERFLOW)
        status |= _SW_OVERFLOW;
    if (mxcsr & _MM_EXCEPT_UNDERFLOW)
        status |= _SW_UNDERFLOW;
    if (mxcsr & _MM_EXCEPT_INEXACT)
        status |= _SW_INEXACT;

    return status;
}
