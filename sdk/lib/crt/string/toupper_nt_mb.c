/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Multibyte capable version of toupper
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <ndk/rtlfuncs.h>

_Check_return_
int
__cdecl
toupper_nt_mb(_In_ int _C)
{
    PUCHAR ptr;
    WCHAR wc, wcUpper;
    CHAR chrUpper[2];
    ULONG mbSize;

    ptr = (PUCHAR)&_C;
    wc = RtlAnsiCharToUnicodeChar(&ptr);
    wcUpper = RtlUpcaseUnicodeChar(wc);
    RtlUnicodeToMultiByteN(chrUpper, 2, &mbSize, &wcUpper, sizeof(WCHAR));

    if (mbSize == 2)
        return (UCHAR)chrUpper[1] + ((UCHAR)chrUpper[0] << 8);
    else if (mbSize == 1)
        return (UCHAR)chrUpper[0];
    else
        return (WCHAR)_C;
}
