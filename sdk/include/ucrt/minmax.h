//
// minmax.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Familiar min() and max() macros
//
#pragma once
#ifndef _INC_MINMAX
#define _INC_MINMAX

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

#ifndef max
    #define max(a ,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
    #define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_MINMAX
