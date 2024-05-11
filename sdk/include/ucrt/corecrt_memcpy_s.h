//
// corecrt_memcpy_s.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Inline definitions of memcpy_s and memmove_s
//
#pragma once

#include <corecrt.h>
#include <errno.h>
#include <vcruntime_string.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

#ifndef _CRT_MEMCPY_S_INLINE
    #define _CRT_MEMCPY_S_INLINE static __inline
#endif

#define _CRT_MEMCPY_S_VALIDATE_RETURN_ERRCODE(expr, errorcode)                 \
    {                                                                          \
        int _Expr_val=!!(expr);                                                \
        if (!(_Expr_val))                                                      \
        {                                                                      \
            errno = errorcode;                                                 \
            _invalid_parameter_noinfo();                                       \
            return errorcode;                                                  \
        }                                                                      \
    }

#if !defined RC_INVOKED && !defined __midl && __STDC_WANT_SECURE_LIB__

    _Success_(return == 0)
    _Check_return_opt_
    _CRT_MEMCPY_S_INLINE errno_t __CRTDECL memcpy_s(
        _Out_writes_bytes_to_opt_(_DestinationSize, _SourceSize) void*       const _Destination,
        _In_                                                     rsize_t     const _DestinationSize,
        _In_reads_bytes_opt_(_SourceSize)                        void const* const _Source,
        _In_                                                     rsize_t     const _SourceSize
        )
    {
        if (_SourceSize == 0)
        {
            return 0;
        }

        _CRT_MEMCPY_S_VALIDATE_RETURN_ERRCODE(_Destination != NULL, EINVAL);
        if (_Source == NULL || _DestinationSize < _SourceSize)
        {
            memset(_Destination, 0, _DestinationSize);

            _CRT_MEMCPY_S_VALIDATE_RETURN_ERRCODE(_Source != NULL,                 EINVAL);
            _CRT_MEMCPY_S_VALIDATE_RETURN_ERRCODE(_DestinationSize >= _SourceSize, ERANGE);

            // Unreachable, but required to suppress /analyze warnings:
            return EINVAL;
        }
        memcpy(_Destination, _Source, _SourceSize);
        return 0;
    }

    _Check_return_wat_
    _CRT_MEMCPY_S_INLINE errno_t __CRTDECL memmove_s(
        _Out_writes_bytes_to_opt_(_DestinationSize, _SourceSize) void*       const _Destination,
        _In_                                                     rsize_t     const _DestinationSize,
        _In_reads_bytes_opt_(_SourceSize)                        void const* const _Source,
        _In_                                                     rsize_t     const _SourceSize
        )
    {
        if (_SourceSize == 0)
        {
            return 0;
        }

        _CRT_MEMCPY_S_VALIDATE_RETURN_ERRCODE(_Destination != NULL,            EINVAL);
        _CRT_MEMCPY_S_VALIDATE_RETURN_ERRCODE(_Source != NULL,                 EINVAL);
        _CRT_MEMCPY_S_VALIDATE_RETURN_ERRCODE(_DestinationSize >= _SourceSize, ERANGE);

        memmove(_Destination, _Source, _SourceSize);
        return 0;
    }

#endif

#undef _CRT_MEMCPY_S_VALIDATE_RETURN_ERRCODE

_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
_CRT_END_C_HEADER
