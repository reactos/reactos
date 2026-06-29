//
// dos.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the structures, constants, and functions used for the
// legacy DOS interface.
//
#pragma once
#ifndef _INC_DOS // include guard for 3rd party interop
#define _INC_DOS

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

// File attribute constants
#define _A_NORMAL 0x00 // Normal file - No read/write restrictions
#define _A_RDONLY 0x01 // Read only file
#define _A_HIDDEN 0x02 // Hidden file
#define _A_SYSTEM 0x04 // System file
#define _A_SUBDIR 0x10 // Subdirectory
#define _A_ARCH   0x20 // Archive file

#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

    #ifndef _DISKFREE_T_DEFINED
        #define _DISKFREE_T_DEFINED

        struct _diskfree_t
        {
            unsigned total_clusters;
            unsigned avail_clusters;
            unsigned sectors_per_cluster;
            unsigned bytes_per_sector;
        };
    #endif

    #if _CRT_FUNCTIONS_REQUIRED
        _Success_(return == 0)
        _Check_return_
        _DCRTIMP unsigned __cdecl _getdiskfree(
            _In_  unsigned            _Drive,
            _Out_ struct _diskfree_t* _DiskFree
            );
    #endif

    #if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES
        #define diskfree_t _diskfree_t
    #endif

#endif

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_DOS
