/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of tolower
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
_CRTIMP
int
__cdecl
tolower(
    _In_ int _C)
{
    if (((char)_C >= 'A') && ((char)_C <= 'Z'))
    {
        return _C + ('a' - 'A');
    }

    return _C;
}
