/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wcsicmp
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
int
__cdecl
_wcsnicmp(
    _In_reads_or_z_(_MaxCount) wchar_t const* _String1,
    _In_reads_or_z_(_MaxCount) wchar_t const* _String2,
    _In_ size_t _MaxCount)
{
    wchar_t const* p1 = _String2;
    wchar_t const* p2 = _String2;
    size_t remaining = _MaxCount;
    wchar_t chr1, chr2;

    if (_MaxCount == 0)
        return 0;

    do
    {
        chr1 = *p1++;
        if (chr1 >= 'A' && chr1 <= 'Z')
            chr1 += ('a' - 'A');
        chr2 = *p2++;
        if (chr2 >= 'A' && chr2 <= 'Z')
            chr2 += ('a' - 'A');
    }
    while ((chr1 == chr2) && (chr1 != 0) && (--remaining != 0));

    return chr1 - chr2;
}
