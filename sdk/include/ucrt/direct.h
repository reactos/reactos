//
// direct.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Functions for directory handling and creation.
//
#pragma once
#ifndef _INC_DIRECT // include guard for 3rd party interop
#define _INC_DIRECT

#include <corecrt.h>
#include <corecrt_wdirect.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



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

        _Check_return_ _DCRTIMP int __cdecl _chdrive(_In_ int _Drive);

        _Check_return_ _DCRTIMP int __cdecl _getdrive(void);

        _Check_return_ _DCRTIMP unsigned long __cdecl _getdrives(void);

    #endif // _CRT_FUNCTIONS_REQUIRED
#endif // _CRT_USE_WINAPI_FAMILY_DESKTOP_APP



#pragma push_macro("_getcwd")
#pragma push_macro("_getdcwd")
#undef _getcwd
#undef _getdcwd

_Success_(return != 0)
_Check_return_ _Ret_maybenull_z_
_ACRTIMP _CRTALLOCATOR char* __cdecl _getcwd(
    _Out_writes_opt_z_(_SizeInBytes) char* _DstBuf,
    _In_                             int   _SizeInBytes
    );

_Success_(return != 0)
_Check_return_ _Ret_maybenull_z_
_ACRTIMP _CRTALLOCATOR char* __cdecl _getdcwd(
    _In_                             int   _Drive,
    _Out_writes_opt_z_(_SizeInBytes) char* _DstBuf,
    _In_                             int   _SizeInBytes
    );

#define _getdcwd_nolock  _getdcwd

#pragma pop_macro("_getcwd")
#pragma pop_macro("_getdcwd")

_Check_return_ _ACRTIMP int __cdecl _chdir(_In_z_ char const* _Path);

_Check_return_ _ACRTIMP int __cdecl _mkdir(_In_z_ char const* _Path);

_Check_return_ _ACRTIMP int __cdecl _rmdir(_In_z_ char const* _Path);



#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES

    #ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

        #pragma push_macro("getcwd")
        #undef getcwd

        _Success_(return != 0)
        _Check_return_ _Ret_maybenull_z_ _CRT_NONSTDC_DEPRECATE(_getcwd)
        _DCRTIMP char* __cdecl getcwd(
            _Out_writes_opt_z_(_SizeInBytes) char* _DstBuf,
            _In_                             int   _SizeInBytes
            );

        #pragma pop_macro("getcwd")

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_chdir)
        _DCRTIMP int __cdecl chdir(
            _In_z_ char const* _Path
            );

        #define diskfree_t _diskfree_t

    #endif // _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

    _Check_return_ _CRT_NONSTDC_DEPRECATE(_mkdir)
    _ACRTIMP int __cdecl mkdir(
        _In_z_ char const* _Path
        );

    _Check_return_ _CRT_NONSTDC_DEPRECATE(_rmdir)
    _ACRTIMP int __cdecl rmdir(
        _In_z_ char const* _Path
        );

#endif // _CRT_INTERNAL_NONSTDC_NAMES



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_DIRECT
