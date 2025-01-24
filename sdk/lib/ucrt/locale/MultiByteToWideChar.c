//
// MultiByteToWideChar.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Definition of __acrt_MultiByteToWideChar.
//
// SPDX-License-Identifier: MIT
//

#include <windows.h>
//#include <stringapiset.h>

_Success_(return != 0)
int
__cdecl
__acrt_MultiByteToWideChar (
    _In_ UINT _CodePage,
    _In_ DWORD _DWFlags,
    _In_ LPCSTR _LpMultiByteStr,
    _In_ int _CbMultiByte,
    _Out_writes_opt_(_CchWideChar) LPWSTR _LpWideCharStr,
    _In_ int _CchWideChar)
{
    return MultiByteToWideChar(_CodePage,
                               _DWFlags,
                               _LpMultiByteStr,
                               _CbMultiByte,
                               _LpWideCharStr,
                               _CchWideChar);
}
