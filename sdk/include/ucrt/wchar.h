//
// wchar.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// All of the types, macros, and function declarations for all wide-character
// related functionality.  Most of the functionality is in the #included
// <corecrt_wxxxx.h> headers, which are also #included by other public headers.
//
#pragma once
#ifndef _INC_WCHAR // include guard for 3rd party interop
#define _INC_WCHAR

#include <corecrt.h>
#include <corecrt_memcpy_s.h>
#include <corecrt_wconio.h>
#include <corecrt_wctype.h>
#include <corecrt_wdirect.h>
#include <corecrt_wio.h>
#include <corecrt_wprocess.h>
#include <corecrt_wstdio.h>
#include <corecrt_wstdlib.h>
#include <corecrt_wstring.h>
#include <corecrt_wtime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vcruntime_string.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



#define WCHAR_MIN 0x0000
#define WCHAR_MAX 0xffff



typedef wchar_t _Wint_t;



#if _CRT_FUNCTIONS_REQUIRED

    _Check_return_opt_ _Success_(return != 0) _Ret_z_
    _ACRTIMP wchar_t* __cdecl _wsetlocale(
        _In_       int            _Category,
        _In_opt_z_ wchar_t const* _Locale
        );

    _Check_return_opt_
    _ACRTIMP _locale_t __cdecl _wcreate_locale(
        _In_   int            _Category,
        _In_z_ wchar_t const* _Locale
        );



    _ACRTIMP wint_t __cdecl btowc(
        _In_ int _Ch
        );

    _ACRTIMP size_t __cdecl mbrlen(
        _In_reads_bytes_opt_(_SizeInBytes) _Pre_opt_z_ char const* _Ch,
        _In_                                           size_t      _SizeInBytes,
        _Inout_                                        mbstate_t*  _State
        );

    _ACRTIMP size_t __cdecl mbrtowc(
        _Pre_maybenull_ _Post_z_                       wchar_t*    _DstCh,
        _In_reads_bytes_opt_(_SizeInBytes) _Pre_opt_z_ char const* _SrcCh,
        _In_                                           size_t      _SizeInBytes,
        _Inout_                                        mbstate_t*  _State
        );

    _Success_(return == 0)
    _ACRTIMP errno_t __cdecl mbsrtowcs_s(
        _Out_opt_                         size_t*      _Retval,
        _Out_writes_opt_z_(_Size)         wchar_t*     _Dst,
        _In_                              size_t       _Size,
        _Deref_pre_opt_z_                 char const** _PSrc,
        _In_                              size_t       _N,
        _Inout_                           mbstate_t*   _State
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_3(
        _Success_(return == 0)
        errno_t, mbsrtowcs_s,
        _Out_opt_                         size_t*,      _Retval,
        _Post_z_                          wchar_t,      _Dest,
        _Inout_ _Deref_prepost_opt_valid_ char const**, _PSource,
        _In_                              size_t,       _Count,
        _Inout_                           mbstate_t*,   _State
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_SIZE(
        _Success_(return == 0) _ACRTIMP, mbsrtowcs,
        _Out_writes_opt_z_(_Count),           wchar_t,      _Dest,
        _Deref_pre_opt_z_                 char const**, _PSrc,
        _In_                              size_t,       _Count,
        _Inout_                           mbstate_t*,   _State
        )

    _Success_(return == 0)
    _ACRTIMP errno_t __cdecl wcrtomb_s(
        _Out_opt_                        size_t*    _Retval,
        _Out_writes_opt_z_(_SizeInBytes) char*      _Dst,
        _In_                             size_t     _SizeInBytes,
        _In_                             wchar_t    _Ch,
        _Inout_opt_                      mbstate_t* _State
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_2(
        _Success_(return == 0)
        errno_t, wcrtomb_s,
        _Out_opt_                 size_t*,    _Retval,
        _Out_writes_opt_z_(_Size) char,       _Dest,
        _In_                      wchar_t,    _Source,
        _Inout_opt_               mbstate_t*, _State
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_SIZE(
        _ACRTIMP, wcrtomb,
        _Pre_maybenull_ _Post_z_, char,       _Dest,
        _In_                      wchar_t,    _Source,
        _Inout_opt_              mbstate_t*, _State
        )

    _Success_(return == 0)
    _ACRTIMP errno_t __cdecl wcsrtombs_s(
        _Out_opt_                                         size_t*         _Retval,
        _Out_writes_bytes_to_opt_(_SizeInBytes, *_Retval) char*           _Dst,
        _In_                                              size_t          _SizeInBytes,
        _Inout_ _Deref_prepost_z_                         wchar_t const** _Src,
        _In_                                              size_t          _Size,
        _Inout_opt_                                       mbstate_t*      _State
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_1_3(
        _Success_(return == 0)
        errno_t, wcsrtombs_s,
        _Out_opt_                 size_t*,         _Retval,
        _Out_writes_opt_z_(_Size) char,            _Dest,
        _Inout_ _Deref_prepost_z_ wchar_t const**, _PSrc,
        _In_                      size_t,          _Count,
        _Inout_opt_               mbstate_t*,      _State
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_3_SIZE(
        _ACRTIMP, wcsrtombs,
        _Pre_maybenull_ _Post_z_, char,            _Dest,
        _Inout_ _Deref_prepost_z_ wchar_t const**, _PSource,
        _In_                      size_t,          _Count,
        _Inout_opt_               mbstate_t*,      _State
        )

    _ACRTIMP int __cdecl wctob(
        _In_ wint_t _WCh
        );

    #if __STDC_WANT_SECURE_LIB__

        _Success_(return == 0)
        errno_t __CRTDECL wmemcpy_s(
            _Out_writes_to_opt_(_N1, _N) wchar_t*       _S1,
            _In_                         rsize_t        _N1,
            _In_reads_opt_(_N)           wchar_t const* _S2,
            _In_                         rsize_t        _N
            );

        _Success_(return == 0)
        errno_t __CRTDECL wmemmove_s(
            _Out_writes_to_opt_(_N1, _N) wchar_t*       _S1,
            _In_                         rsize_t        _N1,
            _In_reads_opt_(_N)           wchar_t const* _S2,
            _In_                         rsize_t        _N
            );

    #endif // __STDC_WANT_SECURE_LIB__

    __inline int __CRTDECL fwide(
        _In_opt_ FILE* _F,
        _In_     int   _M
        )
    {
        _CRT_UNUSED(_F);
        return (_M);
    }

    __inline int __CRTDECL mbsinit(
        _In_opt_ mbstate_t const* _P
        )
    {
        return _P == NULL || _P->_Wchar == 0;
    }

    __inline wchar_t _CONST_RETURN* __CRTDECL wmemchr(
        _In_reads_(_N) wchar_t const* _S,
        _In_           wchar_t        _C,
        _In_           size_t         _N
        )
    {
        for (; 0 < _N; ++_S, --_N)
            if (*_S == _C)
                return (wchar_t _CONST_RETURN*)_S;

        return 0;
    }

    __inline int __CRTDECL wmemcmp(
        _In_reads_(_N) wchar_t const* _S1,
        _In_reads_(_N) wchar_t const* _S2,
        _In_           size_t         _N
        )
    {
        for (; 0 < _N; ++_S1, ++_S2, --_N)
            if (*_S1 != *_S2)
                return *_S1 < *_S2 ? -1 : 1;

        return 0;
    }

    _Post_equal_to_(_S1)
    _At_buffer_(_S1, _Iter_, _N, _Post_satisfies_(_S1[_Iter_] == _S2[_Iter_]))
    __inline _CRT_INSECURE_DEPRECATE_MEMORY(wmemcpy_s)
    wchar_t* __CRTDECL wmemcpy(
        _Out_writes_all_(_N) wchar_t*       _S1,
        _In_reads_(_N)       wchar_t const* _S2,
        _In_                 size_t         _N
        )
    {
        #pragma warning(suppress: 6386) // Buffer overrun
        return (wchar_t*)memcpy(_S1, _S2, _N*sizeof(wchar_t));
    }

    __inline _CRT_INSECURE_DEPRECATE_MEMORY(wmemmove_s)
    wchar_t* __CRTDECL wmemmove(
        _Out_writes_all_opt_(_N) wchar_t*       _S1,
        _In_reads_opt_(_N)       wchar_t const* _S2,
        _In_                     size_t         _N
        )
    {
        #pragma warning(suppress: 6386) // Buffer overrun
        return (wchar_t*)memmove(_S1, _S2, _N*sizeof(wchar_t));
    }

    _Post_equal_to_(_S)
    _At_buffer_(_S, _Iter_, _N, _Post_satisfies_(_S[_Iter_] == _C))
    __inline wchar_t* __CRTDECL wmemset(
        _Out_writes_all_(_N) wchar_t* _S,
        _In_                 wchar_t  _C,
        _In_                 size_t   _N
        )
    {
        wchar_t *_Su = _S;
        for (; 0 < _N; ++_Su, --_N)
        {
            *_Su = _C;
        }
        return _S;
    }

    #ifdef __cplusplus

        extern "C++" inline wchar_t* __CRTDECL wmemchr(
            _In_reads_(_N) wchar_t* _S,
            _In_           wchar_t  _C,
            _In_           size_t   _N
            )
        {
            wchar_t const* const _SC = _S;
            return const_cast<wchar_t*>(wmemchr(_SC, _C, _N));
        }

    #endif // __cplusplus

#endif // _CRT_FUNCTIONS_REQUIRED


_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_WCHAR
