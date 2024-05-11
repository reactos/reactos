//
// corecrt_wprocess.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the wide character (wchar_t) process functionality, shared
// by <process.h> and <wchar.h>.
//
#pragma once

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



#if _CRT_FUNCTIONS_REQUIRED
    #ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

        _DCRTIMP intptr_t __cdecl _wexecl(
            _In_z_ wchar_t const* _FileName,
            _In_z_ wchar_t const* _ArgList,
            ...);

        _DCRTIMP intptr_t __cdecl _wexecle(
            _In_z_ wchar_t const* _FileName,
            _In_z_ wchar_t const* _ArgList,
            ...);

        _DCRTIMP intptr_t __cdecl _wexeclp(
            _In_z_ wchar_t const* _FileName,
            _In_z_ wchar_t const* _ArgList,
            ...);

        _DCRTIMP intptr_t __cdecl _wexeclpe(
            _In_z_ wchar_t const* _FileName,
            _In_z_ wchar_t const* _ArgList,
            ...);

        _DCRTIMP intptr_t __cdecl _wexecv(
            _In_z_ wchar_t const*        _FileName,
            _In_z_ wchar_t const* const* _ArgList
            );

        _DCRTIMP intptr_t __cdecl _wexecve(
            _In_z_     wchar_t const*        _FileName,
            _In_z_     wchar_t const* const* _ArgList,
            _In_opt_z_ wchar_t const* const* _Env
            );

        _DCRTIMP intptr_t __cdecl _wexecvp(
            _In_z_ wchar_t const*        _FileName,
            _In_z_ wchar_t const* const* _ArgList
            );

        _DCRTIMP intptr_t __cdecl _wexecvpe(
            _In_z_     wchar_t const*        _FileName,
            _In_z_     wchar_t const* const* _ArgList,
            _In_opt_z_ wchar_t const* const* _Env
            );

        _DCRTIMP intptr_t __cdecl _wspawnl(
            _In_   int            _Mode,
            _In_z_ wchar_t const* _FileName,
            _In_z_ wchar_t const* _ArgList,
            ...);

        _DCRTIMP intptr_t __cdecl _wspawnle(
            _In_   int            _Mode,
            _In_z_ wchar_t const* _FileName,
            _In_z_ wchar_t const* _ArgList,
            ...);

        _DCRTIMP intptr_t __cdecl _wspawnlp(
            _In_   int            _Mode,
            _In_z_ wchar_t const* _FileName,
            _In_z_ wchar_t const* _ArgList,
            ...);

        _DCRTIMP intptr_t __cdecl _wspawnlpe(
            _In_   int            _Mode,
            _In_z_ wchar_t const* _FileName,
            _In_z_ wchar_t const* _ArgList,
            ...);

        _DCRTIMP intptr_t __cdecl _wspawnv(
            _In_   int                   _Mode,
            _In_z_ wchar_t const*        _FileName,
            _In_z_ wchar_t const* const* _ArgList
            );

        _DCRTIMP intptr_t __cdecl _wspawnve(
            _In_       int                   _Mode,
            _In_z_     wchar_t const*        _FileName,
            _In_z_     wchar_t const* const* _ArgList,
            _In_opt_z_ wchar_t const* const* _Env
            );

        _DCRTIMP intptr_t __cdecl _wspawnvp(
            _In_   int                   _Mode,
            _In_z_ wchar_t const*        _FileName,
            _In_z_ wchar_t const* const* _ArgList
            );

        _DCRTIMP intptr_t __cdecl _wspawnvpe(
            _In_       int                   _Mode,
            _In_z_     wchar_t const*        _FileName,
            _In_z_     wchar_t const* const* _ArgList,
            _In_opt_z_ wchar_t const* const* _Env
            );

        _DCRTIMP int __cdecl _wsystem(
            _In_opt_z_ wchar_t const* _Command
            );

    #endif // _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
#endif // _CRT_FUNCTIONS_REQUIRED



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
