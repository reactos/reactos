/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _isnanf.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <math.h>
#include <stdint.h>

int
__cdecl
_isnanf(_In_ float _X)
{
    union { float f; uint32_t ui32; } u = { _X };
    return (u.ui32 & ~0x80000000) > 0x7F800000;
}
