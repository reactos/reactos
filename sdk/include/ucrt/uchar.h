//
// uchar.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//

#pragma once
#ifndef _UCHAR // include guard for 3rd party interop
#define _UCHAR

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

#ifndef __STDC_UTF_16__
    #define __STDC_UTF_16__ 1
#endif

#ifndef __STDC_UTF_32__
    #define __STDC_UTF_32__ 1
#endif

typedef unsigned short _Char16_t;
typedef unsigned int _Char32_t;

#if !defined __cplusplus || (defined _MSC_VER && _MSC_VER < 1900)
    typedef unsigned short char16_t;
    typedef unsigned int char32_t;
#endif


_Check_return_ _ACRTIMP size_t __cdecl mbrtoc16(_Out_opt_ char16_t *_Pc16, _In_reads_or_z_opt_(_N) const char *_S, _In_ size_t _N, _Inout_ mbstate_t *_Ps);
_Check_return_ _ACRTIMP size_t __cdecl c16rtomb(_Out_writes_opt_(4) char *_S, _In_ char16_t _C16, _Inout_ mbstate_t *_Ps);

_Check_return_ _ACRTIMP size_t __cdecl mbrtoc32(_Out_opt_ char32_t *_Pc32, _In_reads_or_z_opt_(_N) const char *_S, _In_ size_t _N, _Inout_ mbstate_t *_Ps);
_Check_return_ _ACRTIMP size_t __cdecl c32rtomb(_Out_writes_opt_(4) char *_S, _In_ char32_t _C32, _Inout_ mbstate_t *_Ps);

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS

/*
 * Copyright (c) 1992-2013 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V6.40:0009 */
 #endif // _UCHAR
