//
// corecrt_memory.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The buffer (memory) manipulation library.  These declarations are split out
// so that they may be included by both <string.h> and <memory.h>.  <string.h>
// does not include <memory.h> to avoid introducing conflicts with other user
// headers named <memory.h>.
//
#pragma once

#include <corecrt.h>
#include <corecrt_memcpy_s.h>
#include <vcruntime_string.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

#ifndef __midl

_CRT_BEGIN_C_HEADER



_Check_return_
_ACRTIMP int __cdecl _memicmp(
    _In_reads_bytes_opt_(_Size) void const* _Buf1,
    _In_reads_bytes_opt_(_Size) void const* _Buf2,
    _In_                        size_t      _Size
    );

_Check_return_
_ACRTIMP int __cdecl _memicmp_l(
    _In_reads_bytes_opt_(_Size) void const* _Buf1,
    _In_reads_bytes_opt_(_Size) void const* _Buf2,
    _In_                        size_t      _Size,
    _In_opt_                    _locale_t   _Locale
    );



#if !defined RC_INVOKED && __STDC_WANT_SECURE_LIB__

    #if defined __cplusplus && _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY
    extern "C++"
    {
        template <size_t _Size, typename _DstType>
        inline typename _CrtEnableIf<(_Size > 1), void *>::_Type __cdecl memcpy(
            _DstType (&_Dst)[_Size],
            _In_reads_bytes_opt_(_SrcSize) void const* _Src,
            _In_                           size_t      _SrcSize
            ) _CRT_SECURE_CPP_NOTHROW
        {
            return memcpy_s(_Dst, _Size * sizeof(_DstType), _Src, _SrcSize) == 0 ? _Dst : 0;
        }
    }
    #endif

    #if defined __cplusplus && _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY
    extern "C++"
    {
        template <size_t _Size, typename _DstType>
        inline errno_t __CRTDECL memcpy_s(
            _DstType (&_Dst)[_Size],
            _In_reads_bytes_opt_(_SrcSize) void const* _Src,
            _In_                           rsize_t     _SrcSize
            ) _CRT_SECURE_CPP_NOTHROW
        {
            return memcpy_s(_Dst, _Size * sizeof(_DstType), _Src, _SrcSize);
        }
    }
    #endif

#endif // !defined RC_INVOKED && __STDC_WANT_SECURE_LIB__



#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES

    _CRT_NONSTDC_DEPRECATE(_memccpy)
    _ACRTIMP void* __cdecl memccpy(
        _Out_writes_bytes_opt_(_Size) void*       _Dst,
        _In_reads_bytes_opt_(_Size)   void const* _Src,
        _In_                          int         _Val,
        _In_                          size_t      _Size
        );

    _Check_return_ _CRT_NONSTDC_DEPRECATE(_memicmp)
    _ACRTIMP int __cdecl memicmp(
        _In_reads_bytes_opt_(_Size) void const* _Buf1,
        _In_reads_bytes_opt_(_Size) void const* _Buf2,
        _In_                        size_t      _Size
        );

#endif // _CRT_INTERNAL_NONSTDC_NAMES



#if defined __cplusplus

    extern "C++" _Check_return_
    inline void* __CRTDECL memchr(
        _In_reads_bytes_opt_(_N) void*  _Pv,
        _In_                     int    _C,
        _In_                     size_t _N
        )
    {
        void const* const _Pvc = _Pv;
        return const_cast<void*>(memchr(_Pvc, _C, _N));
    }

#endif



_CRT_END_C_HEADER

#endif // !__midl
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
