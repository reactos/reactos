/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of iswctype
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>

extern const unsigned short _wctype[];

_Check_return_
int
__cdecl
iswctype(wint_t _C, wctype_t _Type)
{
    if (_C <= 0xFF)
        return (_wctype[_C + 1] & _Type);

    return 0;
}
