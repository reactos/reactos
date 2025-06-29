/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _dsign.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <math.h>
#include <stdint.h>

_Check_return_
int
__cdecl
_dsign(_In_ double _X)
{
    union { double f; uint64_t ui64; } u = { _X };

    // This is what Windows returns
    return (u.ui64 >> 48) & 0x8000;
}
