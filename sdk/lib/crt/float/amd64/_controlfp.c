/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x64 implementation of _controlfp
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <float.h>

unsigned int __cdecl _controlfp(unsigned int newval, unsigned int mask)
{
    return _control87(newval, mask & ~_EM_DENORMAL);
}
