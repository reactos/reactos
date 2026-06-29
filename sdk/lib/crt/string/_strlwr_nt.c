/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _strlwr
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

char*
__cdecl
_strlwr(
    _Inout_z_ char* _String)
{
    char ch, *p;

    for (p = _String; *p; p++)
    {
        ch = *p;
        if ((ch >= 'A') && (ch <= 'Z'))
        {
            *p += 'a' - 'A';
        }
    }

    return _String;
}
