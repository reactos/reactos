//
// sys/types.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Types used for returning file status and time information.
//
#pragma once

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

#ifndef _INO_T_DEFINED
    #define _INO_T_DEFINED

    typedef unsigned short _ino_t; // inode number (unused on Windows)

    #if (defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__)
        typedef _ino_t ino_t;
    #endif
#endif



#ifndef _DEV_T_DEFINED
    #define _DEV_T_DEFINED

    typedef unsigned int _dev_t; // device code

    #if (defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__)
        typedef _dev_t dev_t;
    #endif
#endif



#ifndef _OFF_T_DEFINED
    #define _OFF_T_DEFINED

    typedef long _off_t; // file offset value

    #if (defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__)
        typedef _off_t off_t;
    #endif
#endif

_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
