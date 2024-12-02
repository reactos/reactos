//
// WideCharToMultiByte.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Definition of __acrt_WideCharToMultiByte.
//
// SPDX-License-Identifier: MIT
//

#include <windows.h>
//#include <stringapiset.h>

_Success_(return != 0)
int
__cdecl
__acrt_WideCharToMultiByte (
    _In_ UINT _CodePage,
    _In_ DWORD _DWFlags,
    _In_ LPCWSTR _LpWideCharStr,
    _In_ int _CchWideChar,
    _Out_writes_opt_(_CbMultiByte) LPSTR _LpMultiByteStr,
    _In_ int _CbMultiByte,
    _In_opt_ LPCSTR  _LpDefaultChar,
    _Out_opt_ LPBOOL  _LpUsedDefaultChar)
{
    return WideCharToMultiByte(_CodePage,
                               _DWFlags,
                               _LpWideCharStr,
                               _CchWideChar,
                               _LpMultiByteStr,
                               _CbMultiByte,
                               _LpDefaultChar,
                               _LpUsedDefaultChar);
}
