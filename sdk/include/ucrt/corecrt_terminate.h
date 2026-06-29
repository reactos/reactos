//
// corecrt_terminate.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The terminate handler
//
#pragma once

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

#ifndef RC_INVOKED

_CRT_BEGIN_C_HEADER

// terminate_handler is the standard name; terminate_function is defined for
// source compatibility.
typedef void (__CRTDECL* terminate_handler )(void);
typedef void (__CRTDECL* terminate_function)(void);

#ifdef _M_CEE
    typedef void (__clrcall* __terminate_function_m)();
    typedef void (__clrcall* __terminate_handler_m )();
#endif

#ifdef __cplusplus

    _ACRTIMP __declspec(noreturn) void __cdecl abort();
    _ACRTIMP __declspec(noreturn) void __cdecl terminate() throw();

    #ifndef _M_CEE_PURE

        _ACRTIMP terminate_handler __cdecl set_terminate(
            _In_opt_ terminate_handler _NewTerminateHandler
            ) throw();

        _ACRTIMP terminate_handler __cdecl _get_terminate();

    #endif

#endif // __cplusplus

_CRT_END_C_HEADER

#endif // RC_INVOKED
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
