/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of tolower
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <ndk/rtlfuncs.h>

_Check_return_
int
__cdecl
tolower_nt(_In_ int _C)
{
    PUCHAR ptr;
    WCHAR wc, wcLower;
    CHAR chrLower[2];
    ULONG mbSize;
 
    ptr = (PUCHAR)&_C;
    wc = RtlAnsiCharToUnicodeChar(&ptr);
    wcLower = RtlDowncaseUnicodeChar(wc);
    RtlUnicodeToMultiByteN(chrLower, 2, &mbSize, &wcLower, sizeof(WCHAR));

    if (mbSize == 2)
        return (UCHAR)chrLower[1] + ((UCHAR)chrLower[0] << 8);
    else if (mbSize == 1)
        return (UCHAR)chrLower[0];
    else
        return (WCHAR)_C;
}

#undef _tolower
_Check_return_
int
__cdecl
_tolower(_In_ int _C)
{
    return _C + ('a' - 'A');
}
