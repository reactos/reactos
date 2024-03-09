/*
 * PROJECT:     GCC c++ support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     StringCchCopyNA implementation
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <windows.h>
#define StringCchCopyNA _StringCchCopyNA
#include <strsafe.h>

#undef StringCchCopyNA

HRESULT
WINAPI
StringCchCopyNA(
    _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPSTR pszDest,
    _In_ size_t cchDest,
    _In_reads_or_z_(cchToCopy) STRSAFE_LPCSTR pszSrc,
    _In_ size_t cchToCopy)
{
    return _StringCchCopyNA(pszDest, cchDest, pszSrc, cchToCopy);
}
