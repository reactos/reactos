//
// sys/locking.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file defines the flags for the locking() function.
//
#pragma once

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

#define _LK_UNLCK  0  // unlock the file region
#define _LK_LOCK   1  // lock the file region
#define _LK_NBLCK  2  // non-blocking lock
#define _LK_RLCK   3  // lock for writing
#define _LK_NBRLCK 4  // non-blocking lock for writing

#if (defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__)
    #define LK_UNLCK  _LK_UNLCK
    #define LK_LOCK   _LK_LOCK
    #define LK_NBLCK  _LK_NBLCK
    #define LK_RLCK   _LK_RLCK
    #define LK_NBRLCK _LK_NBRLCK
#endif

_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
