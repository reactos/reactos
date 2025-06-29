/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _finite.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <float.h>
#include <stdint.h>

_Check_return_
int
__cdecl
_finite(_In_ double _X)
{
    union { double f; uint64_t ui64; } u = { _X };
    uint64_t exp = u.ui64 & 0x7FF0000000000000;
    return (exp != 0x7FF0000000000000);
}
