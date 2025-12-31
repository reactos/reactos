/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _stricmp
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
int
__cdecl
_stricmp(
    _In_z_ char const* _String1,
    _In_z_ char const* _String2)
{
    char const* p1 = _String1;
    char const* p2 = _String2;
    char chr1, chr2;

    while (1)
    {
        chr1 = *p1++;
        chr2 = *p2++;

        if (chr1 != chr2)
        {
            if ((chr1 >= 'A') && (chr1 <= 'Z'))
                chr1 += ('a' - 'A');
            if ((chr2 >= 'A') && (chr2 <= 'Z'))
                chr2 += ('a' - 'A');

            if (chr1 != chr2)
                return chr1 - chr2;
        }
        else if (chr1 == 0)
        {
            break;
        }
    }

    return 0;
}
