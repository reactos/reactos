/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 implementation of _controlfp
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <float.h>

unsigned int __cdecl _controlfp(unsigned int newval, unsigned int mask)
{
    /* The denormal exception mask is only reachable through _control87 */
    return _control87(newval, mask & ~_EM_DENORMAL);
}
