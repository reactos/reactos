/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of x64 floating point control word helper functions
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <float.h>
#include <xmmintrin.h>

#define _MM_DENORMALS_ARE_ZERO 0x0040

unsigned int _get_native_fpcw(void)
{
    return _mm_getcsr();
}

void _set_native_fpcw(unsigned int value)
{
    _mm_setcsr(value);
}

unsigned int _fpcw_native_to_abstract(unsigned int native)
{
    unsigned int rounding_mask, abstract = 0;

    /* Handle exception mask */
    if (native & _MM_MASK_INVALID)
        abstract |= _EM_INVALID;
    if (native & _MM_MASK_DENORM)
        abstract |= _EM_DENORMAL;
    if (native & _MM_MASK_DIV_ZERO)
        abstract |= _EM_ZERODIVIDE;
    if (native & _MM_MASK_OVERFLOW)
        abstract |= _EM_OVERFLOW;
    if (native & _MM_MASK_UNDERFLOW)
        abstract |= _EM_UNDERFLOW;
    if (native & _MM_MASK_INEXACT)
        abstract |= _EM_INEXACT;

    /* Handle rounding mode */
    rounding_mask = (native & _MM_ROUND_MASK);
    if (rounding_mask == _MM_ROUND_DOWN)
        abstract |= _RC_DOWN;
    else if(rounding_mask == _MM_ROUND_UP)
        abstract |= _RC_UP;
    else if (rounding_mask == _MM_ROUND_TOWARD_ZERO)
        abstract |= _RC_CHOP;

    /* Handle denormal control */
    if (native & _MM_DENORMALS_ARE_ZERO)
    {
        if (native & _MM_FLUSH_ZERO_MASK)
            abstract |= _DN_FLUSH;
        else
            abstract |= _DN_FLUSH_OPERANDS_SAVE_RESULTS;
    }
    else
    {
        if (native & _MM_FLUSH_ZERO_MASK)
            abstract |= _DN_SAVE_OPERANDS_FLUSH_RESULTS;
        else
            abstract |= _DN_SAVE;
    }

    return abstract;
}

unsigned int _fpcw_abstract_to_native(unsigned int abstract)
{
    unsigned int rounding_mask, native = 0;

    /* Handle exception mask */
    if (abstract & _EM_INVALID)
        native |= _MM_MASK_INVALID;
    if (abstract & _EM_DENORMAL)
        native |= _MM_MASK_DENORM;
    if (abstract & _EM_ZERODIVIDE)
        native |= _MM_MASK_DIV_ZERO;
    if (abstract & _EM_OVERFLOW)
        native |= _MM_MASK_OVERFLOW;
    if (abstract & _EM_UNDERFLOW)
        native |= _MM_MASK_UNDERFLOW;
    if (abstract & _EM_INEXACT)
        native |= _MM_MASK_INEXACT;

    /* Handle rounding mode */
    rounding_mask = (abstract & _MCW_RC);
    if (rounding_mask == _RC_DOWN)
        native |= _MM_ROUND_DOWN;
    else if (rounding_mask == _RC_UP)
        native |= _MM_ROUND_UP;
    else if (rounding_mask == _RC_CHOP)
        native |= _MM_ROUND_TOWARD_ZERO;

    /* Handle Denormal Control */
    if ((abstract & _MCW_DN) == _DN_FLUSH)
    {
        native |= _MM_DENORMALS_ARE_ZERO | _MM_FLUSH_ZERO_MASK;
    }
    else if ((abstract & _MCW_DN) == _DN_FLUSH_OPERANDS_SAVE_RESULTS)
    {
        native |= _MM_DENORMALS_ARE_ZERO;
    }
    else if ((abstract & _MCW_DN) == _DN_SAVE_OPERANDS_FLUSH_RESULTS)
    {
        native |= _MM_FLUSH_ZERO_MASK;
    }

    return native;
}
