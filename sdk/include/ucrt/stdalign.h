//
// stdalign.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The C Standard Library <stdalign.h> header.
//
#pragma once
#ifndef _INC_STDALIGN // include guard for 3rd party interop
#define _INC_STDALIGN

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS
_CRT_BEGIN_C_HEADER

#if _CRT_HAS_C11

#define alignas _Alignas
#define alignof _Alignof
#define __alignas_is_defined 1
#define __alignof_is_defined 1

#endif // _CRT_HAS_C11

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_STDALIGN
