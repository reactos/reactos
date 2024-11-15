/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _isnan.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <float.h>
#include <stdint.h>

int
__cdecl
_isnan(_In_ double _X)
{
    union { double f; uint64_t ui64; } u = { _X };
    return (u.ui64 & ~0x8000000000000000ull) > 0x7FF0000000000000ull;
}
