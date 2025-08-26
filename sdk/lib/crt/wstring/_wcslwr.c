/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wcslwr
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

wchar_t*
__cdecl
_wcslwr(
    _Inout_z_ wchar_t* _String)
{
    wchar_t ch, *p;

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
