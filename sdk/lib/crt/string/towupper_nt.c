/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of towupper
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <ndk/rtlfuncs.h>

_Check_return_
_CRTIMP
wint_t
__cdecl
towupper(
    _In_ wint_t _C)
{
    return RtlUpcaseUnicodeChar((WCHAR)_C);
}
