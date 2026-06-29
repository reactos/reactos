/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _fdsign.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <math.h>
#include <stdint.h>

_Check_return_
int
__cdecl
_fdsign(_In_ float _X)
{
    union { float f; uint32_t ui32; } u = { _X };

    // This is what Windows returns
    return (u.ui32 >> 16) & 0x8000;
}
