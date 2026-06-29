//
// stdio.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The C Standard Library <wctype.h> header.
//
#pragma once
#ifndef _INC_WCTYPE // include guard for 3rd party interop
#define _INC_WCTYPE

#include <corecrt.h>
#include <corecrt_wctype.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



typedef wchar_t wctrans_t;
_ACRTIMP wint_t __cdecl towctrans(wint_t c, wctrans_t value);
_ACRTIMP wctrans_t __cdecl wctrans(const char *name);
_ACRTIMP wctype_t __cdecl wctype(const char *name);



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_WCTYPE
