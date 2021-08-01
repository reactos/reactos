/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _controlfp
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#include <precomp.h>
#include "fpscr.h"

unsigned int CDECL _controlfp(unsigned int newval, unsigned int mask)
{
    return _control87(newval, mask & ~_EM_DENORMAL);
}

unsigned int CDECL _control87(unsigned int newval, unsigned int mask)
{
    ARM_FPSCR fpscr;
    unsigned int flags = 0;

    TRACE("(%08x, %08x): Called\n", newval, mask);

    /* Get fp control word */
    fpscr.raw = __getfp();

    TRACE("Control word before : %08x\n", fpscr.raw);

    /* Convert into mask constants */
    if (!(fpscr.data.ex_control & ARM_CW_IM)) flags |= _EM_INVALID;
    if (!(fpscr.data.ex_control & ARM_CW_ZM)) flags |= _EM_ZERODIVIDE;
    if (!(fpscr.data.ex_control & ARM_CW_OM)) flags |= _EM_OVERFLOW;
    if (!(fpscr.data.ex_control & ARM_CW_UM)) flags |= _EM_UNDERFLOW;
    if (!(fpscr.data.ex_control & ARM_CW_PM)) flags |= _EM_INEXACT;
    if (!(fpscr.data.ex_control & ARM_CW_DM)) flags |= _EM_DENORMAL;

    switch (fpscr.data.rounding_mode)
    {
        case ARM_CW_RC_ZERO: flags |= _RC_UP|_RC_DOWN; break;
        case ARM_CW_RC_UP:   flags |= _RC_UP; break;
        case ARM_CW_RC_DOWN: flags |= _RC_DOWN; break;
    }

    /* Mask with parameters */
    flags = (flags & ~mask) | (newval & mask);

    /* Convert (masked) value back to fp word */
    fpscr.raw = 0;
    if (!(flags & _EM_INVALID))    fpscr.data.ex_control |= ARM_CW_IM;
    if (!(flags & _EM_ZERODIVIDE)) fpscr.data.ex_control |= ARM_CW_ZM;
    if (!(flags & _EM_OVERFLOW))   fpscr.data.ex_control |= ARM_CW_OM;
    if (!(flags & _EM_UNDERFLOW))  fpscr.data.ex_control |= ARM_CW_UM;
    if (!(flags & _EM_INEXACT))    fpscr.data.ex_control |= ARM_CW_PM;
    if (!(flags & _EM_DENORMAL))   fpscr.data.ex_control |= ARM_CW_DM;

    switch (flags & (_RC_UP | _RC_DOWN))
    {
        case _RC_UP|_RC_DOWN: fpscr.data.rounding_mode = ARM_CW_RC_ZERO; break;
        case _RC_UP:          fpscr.data.rounding_mode = ARM_CW_RC_UP; break;
        case _RC_DOWN:        fpscr.data.rounding_mode = ARM_CW_RC_DOWN; break;
        case _RC_NEAR:        fpscr.data.rounding_mode = ARM_CW_RC_NEAREST; break;
    }

    TRACE("Control word after  : %08x\n", fpscr.raw);

    /* Put fp control word */
    __setfp(fpscr.raw);

    return flags;
}

int CDECL _controlfp_s(unsigned int *cur, unsigned int newval, unsigned int mask)
{
    unsigned int val;

    if (!MSVCRT_CHECK_PMT( !(newval & mask & ~(_MCW_EM | _MCW_RC | _MCW_DN)) ))
    {
        if (cur) *cur = _controlfp(0, 0);  /* retrieve it anyway */
        return EINVAL;
    }
    val = _controlfp(newval, mask);
    if (cur) *cur = val;
    return 0;
}
