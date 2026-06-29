/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _finitef.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <float.h>
#include <stdint.h>

_Check_return_
int
__cdecl
_finitef(_In_ float _X)
{
    union { float f; uint32_t ui32; } u = { _X };
    uint32_t exp = u.ui32 & 0x7F800000;
    return (exp != 0x7F800000);
}
