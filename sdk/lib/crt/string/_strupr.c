/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _strupr
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

char*
__cdecl
_strupr(
    _Inout_z_ char* _String)
{
    char ch, *p;

    for (p = _String; *p; p++)
    {
        ch = *p;
        if ((ch >= 'a') && (ch <= 'z'))
        {
            *p += 'A' - 'a';
        }
    }

    return _String;
}
