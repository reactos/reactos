//
// corecrt_wdirect.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the wide character (wchar_t) directory functionality, shared
// by <direct.h> and <wchar.h>.
//
#pragma once

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

#pragma push_macro("_wgetcwd")
#pragma push_macro("_wgetdcwd")
#undef _wgetcwd
#undef _wgetdcwd

_Success_(return != 0)
_Check_return_ _Ret_maybenull_z_
_ACRTIMP _CRTALLOCATOR wchar_t* __cdecl _wgetcwd(
    _Out_writes_opt_z_(_SizeInWords) wchar_t* _DstBuf,
    _In_                             int      _SizeInWords
    );

_Success_(return != 0)
_Check_return_ _Ret_maybenull_z_
_ACRTIMP _CRTALLOCATOR wchar_t* __cdecl _wgetdcwd(
    _In_                             int      _Drive,
    _Out_writes_opt_z_(_SizeInWords) wchar_t* _DstBuf,
    _In_                             int      _SizeInWords
    );

#define _wgetdcwd_nolock  _wgetdcwd

#pragma pop_macro("_wgetcwd")
#pragma pop_macro("_wgetdcwd")

_Check_return_
_ACRTIMP int __cdecl _wchdir(
    _In_z_ wchar_t const* _Path
    );

_Check_return_
_ACRTIMP int __cdecl _wmkdir(
    _In_z_ wchar_t const* _Path
    );

_Check_return_
_ACRTIMP int __cdecl _wrmdir(
    _In_z_ wchar_t const* _Path
    );



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
