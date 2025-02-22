/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x64 implementation of _fpreset
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <xmmintrin.h>

void __cdecl _fpreset(void)
{
    /* Mask everything */
    _mm_setcsr(_MM_MASK_MASK);
}
