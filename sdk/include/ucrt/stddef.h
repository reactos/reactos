//
// stddef.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The C <stddef.h> Standard Library header.
//
#pragma once
#ifndef _INC_STDDEF // include guard for 3rd party interop
#define _INC_STDDEF

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



#ifdef __cplusplus
    namespace std
    {
        typedef decltype(__nullptr) nullptr_t;
    }

    using ::std::nullptr_t;
#endif



#if _CRT_FUNCTIONS_REQUIRED

    _ACRTIMP int* __cdecl _errno(void);
    #define errno (*_errno())

    _ACRTIMP errno_t __cdecl _set_errno(_In_ int _Value);
    _ACRTIMP errno_t __cdecl _get_errno(_Out_ int* _Value);

#endif // _CRT_FUNCTIONS_REQUIRED



#if defined _MSC_VER && !defined _CRT_USE_BUILTIN_OFFSETOF
    #ifdef __cplusplus
        #define offsetof(s,m) ((::size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
    #else
        #define offsetof(s,m) ((size_t)&(((s*)0)->m))
    #endif
#else
    #define offsetof(s,m) __builtin_offsetof(s,m)
#endif

_ACRTIMP extern unsigned long  __cdecl __threadid(void);
#define _threadid (__threadid())
_ACRTIMP extern uintptr_t __cdecl __threadhandle(void);



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_STDDEF
