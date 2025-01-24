//
// GetStringTypeW.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Definition of __acrt_GetStringTypeW.
//
// SPDX-License-Identifier: MIT
//

#include <windows.h>
//#include <stringapiset.h>

_Success_(return)
BOOL
__cdecl
__acrt_GetStringTypeW (
    _In_ DWORD _DWInfoType,
    _In_NLS_string_(_CchSrc) PCWCH _LpSrcStr,
    _In_ int _CchSrc,
    _Out_ LPWORD _LpCharType)
{
    return GetStringTypeW(_DWInfoType, _LpSrcStr, _CchSrc, _LpCharType);
}
