/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Implementation of _controlfp_s (adapted from wine msvcrt/math.c)
 * COPYRIGHT:   Copyright 2000 Jon Griffiths
 *              Copyright 2010 Piotr Caban
 *              Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#include <precomp.h>
#include <float.h>

#ifdef _M_ARM
#define INVALID_MASK ~(_MCW_EM | _MCW_RC | _MCW_DN)
#else
#define INVALID_MASK ~(_MCW_EM | _MCW_IC | _MCW_RC | _MCW_PC | _MCW_DN)
#endif

int CDECL _controlfp_s(unsigned int* cur, unsigned int newval, unsigned int mask)
{
    unsigned int val;

    if (!MSVCRT_CHECK_PMT((newval & mask & INVALID_MASK) == 0))
    {
        if (cur) *cur = _controlfp(0, 0);  /* retrieve it anyway */
        return EINVAL;
    }
    val = _controlfp(newval, mask);
    if (cur) *cur = val;
    return 0;
}
