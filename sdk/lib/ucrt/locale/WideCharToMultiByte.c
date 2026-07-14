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

//
// For flags documentation, see https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
//
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
    /* For UTF-8 and CP 54936 only MB_ERR_INVALID_CHARS is allowed */
    if ((_CodePage == CP_UTF8) || (_CodePage == 54936))
    {
        _DWFlags &= MB_ERR_INVALID_CHARS;
    }

    /* The following code pages require _DWFlags to be 0 */
    else if ((_CodePage == 50220) || (_CodePage == 50221) || (_CodePage == 50222) ||
             (_CodePage == 50225) || (_CodePage == 50227) || (_CodePage == 50229) ||
             ((_CodePage >= 57002) && (_CodePage <= 57011)) ||
             (_CodePage == CP_UTF7) || (_CodePage == CP_SYMBOL))
    {
        _DWFlags = 0;
    }

    return WideCharToMultiByte(_CodePage,
                               _DWFlags,
                               _LpWideCharStr,
                               _CchWideChar,
                               _LpMultiByteStr,
                               _CbMultiByte,
                               _LpDefaultChar,
                               _LpUsedDefaultChar);
}
