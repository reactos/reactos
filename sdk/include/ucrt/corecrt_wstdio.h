//
// corecrt_wstdio.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the wide character (wchar_t) I/O functionality, shared by
// <stdio.h> and <wchar.h>.  It also defines several core I/O types, which are
// also shared by those two headers.
//
#pragma once

#include <corecrt.h>
#include <corecrt_stdio_config.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Stream I/O Declarations Required by this Header
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifndef _FILE_DEFINED
    #define _FILE_DEFINED
    typedef struct _iobuf
    {
        void* _Placeholder;
    } FILE;
#endif

_ACRTIMP_ALT FILE* __cdecl __acrt_iob_func(unsigned _Ix);

#define stdin  (__acrt_iob_func(0))
#define stdout (__acrt_iob_func(1))
#define stderr (__acrt_iob_func(2))

#define WEOF ((wint_t)(0xFFFF))



#if _CRT_FUNCTIONS_REQUIRED
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Wide Character Stream I/O Functions
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_opt_
    _ACRTIMP wint_t __cdecl fgetwc(
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP wint_t __cdecl _fgetwchar(void);

    _Check_return_opt_
    _ACRTIMP wint_t __cdecl fputwc(
        _In_    wchar_t _Character,
        _Inout_ FILE*   _Stream);

    _Check_return_opt_
    _ACRTIMP wint_t __cdecl _fputwchar(
        _In_ wchar_t _Character
        );

    _Check_return_
    _ACRTIMP wint_t __cdecl getwc(
        _Inout_ FILE* _Stream
        );

    _Check_return_
    _ACRTIMP wint_t __cdecl getwchar(void);


    _Check_return_opt_
    _Success_(return == _Buffer)
    _ACRTIMP wchar_t* __cdecl fgetws(
        _Out_writes_z_(_BufferCount) wchar_t* _Buffer,
        _In_                         int      _BufferCount,
        _Inout_                      FILE*    _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl fputws(
        _In_z_  wchar_t const* _Buffer,
        _Inout_ FILE*          _Stream
        );

    _Check_return_opt_
    _Success_(return != 0)
    _ACRTIMP wchar_t* __cdecl _getws_s(
        _Out_writes_z_(_BufferCount) wchar_t* _Buffer,
        _In_                         size_t   _BufferCount
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(
        _Success_(return != 0)
        wchar_t*, _getws_s,
        _Always_(_Post_z_) wchar_t, _Buffer
        )

    _Check_return_opt_
    _ACRTIMP wint_t __cdecl putwc(
        _In_    wchar_t _Character,
        _Inout_ FILE*   _Stream
        );

    _Check_return_opt_
    _ACRTIMP wint_t __cdecl putwchar(
        _In_ wchar_t _Character
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _putws(
        _In_z_ wchar_t const* _Buffer
        );

    _Check_return_opt_
    _ACRTIMP wint_t __cdecl ungetwc(
        _In_    wint_t _Character,
        _Inout_ FILE*  _Stream
        );

    _Check_return_
    _ACRTIMP FILE * __cdecl _wfdopen(
        _In_   int            _FileHandle,
        _In_z_ wchar_t const* _Mode
        );

    _Check_return_ _CRT_INSECURE_DEPRECATE(_wfopen_s)
    _ACRTIMP FILE* __cdecl _wfopen(
        _In_z_ wchar_t const* _FileName,
        _In_z_ wchar_t const* _Mode
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _wfopen_s(
        _Outptr_result_maybenull_ FILE**         _Stream,
        _In_z_                    wchar_t const* _FileName,
        _In_z_                    wchar_t const* _Mode
        );

    _Check_return_
    _CRT_INSECURE_DEPRECATE(_wfreopen_s)
    _ACRTIMP FILE* __cdecl _wfreopen(
        _In_z_  wchar_t const* _FileName,
        _In_z_  wchar_t const* _Mode,
        _Inout_ FILE*          _OldStream
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _wfreopen_s(
        _Outptr_result_maybenull_ FILE**         _Stream,
        _In_z_                    wchar_t const* _FileName,
        _In_z_                    wchar_t const* _Mode,
        _Inout_                   FILE*          _OldStream
        );

    _Check_return_
    _ACRTIMP FILE* __cdecl _wfsopen(
        _In_z_ wchar_t const* _FileName,
        _In_z_ wchar_t const* _Mode,
        _In_   int            _ShFlag
        );

    _ACRTIMP void __cdecl _wperror(
        _In_opt_z_ wchar_t const* _ErrorMessage
        );

    #ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

        _Check_return_
        _DCRTIMP FILE* __cdecl _wpopen(
            _In_z_ wchar_t const* _Command,
            _In_z_ wchar_t const* _Mode
            );

    #endif

    _ACRTIMP int __cdecl _wremove(
        _In_z_ wchar_t const* _FileName
        );

    #pragma push_macro("_wtempnam")
    #undef _wtempnam

    _Check_return_
    _ACRTIMP _CRTALLOCATOR wchar_t* __cdecl _wtempnam(
        _In_opt_z_ wchar_t const* _Directory,
        _In_opt_z_ wchar_t const* _FilePrefix
        );

    #pragma pop_macro("_wtempnam")

    _Success_(return == 0)
    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _wtmpnam_s(
        _Out_writes_z_(_BufferCount) wchar_t* _Buffer,
        _In_                         size_t   _BufferCount
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(
        _Success_(return == 0)
        errno_t, _wtmpnam_s,
        _Always_(_Post_z_) wchar_t, _Buffer
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(
        _Success_(return != 0)
        wchar_t*, __RETURN_POLICY_DST, _ACRTIMP, _wtmpnam,
        _Pre_maybenull_ _Always_(_Post_z_), wchar_t, _Buffer
        )



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // I/O Synchronization and _nolock family of I/O functions
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_opt_
    _ACRTIMP wint_t __cdecl _fgetwc_nolock(
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP wint_t __cdecl _fputwc_nolock(
        _In_    wchar_t _Character,
        _Inout_ FILE*   _Stream
        );

    _Check_return_opt_
    _ACRTIMP wint_t __cdecl _getwc_nolock(
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP wint_t __cdecl _putwc_nolock(
        _In_    wchar_t _Character,
        _Inout_ FILE*   _Stream
        );

    _Check_return_opt_
    _ACRTIMP wint_t __cdecl _ungetwc_nolock(
        _In_    wint_t _Character,
        _Inout_ FILE*  _Stream
        );

    #if defined _CRT_DISABLE_PERFCRIT_LOCKS && !defined _DLL
        #define fgetwc(stream)     _getwc_nolock(stream)
        #define fputwc(c, stream)  _putwc_nolock(c, stream)
        #define ungetwc(c, stream) _ungetwc_nolock(c, stream)
    #endif



    // Variadic functions are not supported in managed code under /clr
    #ifdef _M_CEE_MIXED
        #pragma managed(push, off)
    #endif



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Wide Character Formatted Output Functions (Stream)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_opt_
    _ACRTIMP int __cdecl __stdio_common_vfwprintf(
        _In_                                    unsigned __int64 _Options,
        _Inout_                                 FILE*            _Stream,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl __stdio_common_vfwprintf_s(
        _In_                                    unsigned __int64 _Options,
        _Inout_                                 FILE*            _Stream,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl __stdio_common_vfwprintf_p(
        _In_                                    unsigned __int64 _Options,
        _Inout_                                 FILE*            _Stream,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfwprintf_l(
        _Inout_                                 FILE*          const _Stream,
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vfwprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, _Stream, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vfwprintf(
        _Inout_                       FILE*          const _Stream,
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwprintf_l(_Stream, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfwprintf_s_l(
        _Inout_                                 FILE*          const _Stream,
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vfwprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, _Stream, _Format, _Locale, _ArgList);
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vfwprintf_s(
            _Inout_                       FILE*          const _Stream,
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                          va_list              _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vfwprintf_s_l(_Stream, _Format, NULL, _ArgList);
        }
    #endif

    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfwprintf_p_l(
        _Inout_                                 FILE*          const _Stream,
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vfwprintf_p(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, _Stream, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfwprintf_p(
        _Inout_                       FILE*          const _Stream,
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwprintf_p_l(_Stream, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vwprintf_l(
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwprintf_l(stdout, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vwprintf(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwprintf_l(stdout, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vwprintf_s_l(
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwprintf_s_l(stdout, _Format, _Locale, _ArgList);
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vwprintf_s(
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                          va_list              _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vfwprintf_s_l(stdout, _Format, NULL, _ArgList);
        }
    #endif

    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vwprintf_p_l(
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwprintf_p_l(stdout, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vwprintf_p(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwprintf_p_l(stdout, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _fwprintf_l(
        _Inout_                                 FILE*          const _Stream,
        _In_z_ _Printf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfwprintf_l(_Stream, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL fwprintf(
        _Inout_                       FILE*          const _Stream,
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfwprintf_l(_Stream, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _fwprintf_s_l(
        _Inout_                                 FILE*          const _Stream,
        _In_z_ _Printf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfwprintf_s_l(_Stream, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL fwprintf_s(
            _Inout_                       FILE*          const _Stream,
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vfwprintf_s_l(_Stream, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _fwprintf_p_l(
        _Inout_                                 FILE*          const _Stream,
        _In_z_ _Printf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfwprintf_p_l(_Stream, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _fwprintf_p(
        _Inout_                       FILE*          const _Stream,
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfwprintf_p_l(_Stream, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _wprintf_l(
        _In_z_ _Printf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfwprintf_l(stdout, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL wprintf(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfwprintf_l(stdout, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _wprintf_s_l(
        _In_z_ _Printf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfwprintf_s_l(stdout, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL wprintf_s(
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vfwprintf_s_l(stdout, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _wprintf_p_l(
        _In_z_ _Printf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfwprintf_p_l(stdout, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _wprintf_p(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfwprintf_p_l(stdout, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif


    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Wide Character Formatted Input Functions (Stream)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_opt_
    _ACRTIMP int __cdecl __stdio_common_vfwscanf(
        _In_                                   unsigned __int64 _Options,
        _Inout_                                FILE*            _Stream,
        _In_z_ _Scanf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                               _locale_t        _Locale,
                                               va_list          _ArgList
        );

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfwscanf_l(
        _Inout_ FILE*                                const _Stream,
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        _In_opt_                      _locale_t      const _Locale,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vfwscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS,
            _Stream, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vfwscanf(
        _Inout_ FILE*                                const _Stream,
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwscanf_l(_Stream, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfwscanf_s_l(
        _Inout_                       FILE*          const _Stream,
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        _In_opt_                      _locale_t      const _Locale,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vfwscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT,
            _Stream, _Format, _Locale, _ArgList);
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vfwscanf_s(
            _Inout_                       FILE*          const _Stream,
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                          va_list              _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vfwscanf_s_l(_Stream, _Format, NULL, _ArgList);
        }
    #endif

    #endif

    _CRT_STDIO_INLINE int __CRTDECL _vwscanf_l(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        _In_opt_                      _locale_t      const _Locale,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwscanf_l(stdin, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vwscanf(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwscanf_l(stdin, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vwscanf_s_l(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        _In_opt_                      _locale_t      const _Locale,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfwscanf_s_l(stdin, _Format, _Locale, _ArgList);
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vwscanf_s(
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                          va_list              _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vfwscanf_s_l(stdin, _Format, NULL, _ArgList);
        }
    #endif

    #endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_fwscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _fwscanf_l(
        _Inout_                                FILE*          const _Stream,
        _In_z_ _Scanf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                               _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfwscanf_l(_Stream, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_ _CRT_INSECURE_DEPRECATE(fwscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL fwscanf(
        _Inout_                      FILE*          const _Stream,
        _In_z_ _Scanf_format_string_ wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfwscanf_l(_Stream, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _fwscanf_s_l(
        _Inout_                                  FILE*          const _Stream,
        _In_z_ _Scanf_s_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                 _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfwscanf_s_l(_Stream, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL fwscanf_s(
            _Inout_                        FILE*          const _Stream,
            _In_z_ _Scanf_s_format_string_ wchar_t const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vfwscanf_s_l(_Stream, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_wscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _wscanf_l(
        _In_z_ _Scanf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                               _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfwscanf_l(stdin, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_ _CRT_INSECURE_DEPRECATE(wscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL wscanf(
        _In_z_ _Scanf_format_string_ wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfwscanf_l(stdin, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _wscanf_s_l(
        _In_z_ _Scanf_s_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                 _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfwscanf_s_l(stdin, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL wscanf_s(
            _In_z_ _Scanf_s_format_string_ wchar_t const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
            ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vfwscanf_s_l(stdin, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Wide Character Formatted Output Functions (String)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    #ifndef _CRT_NON_CONFORMING_SWPRINTFS
        #define _SWPRINTFS_DEPRECATED _CRT_DEPRECATE_TEXT(                       \
                "function has been changed to conform with the ISO C standard, " \
                "adding an extra character count parameter. To use the traditional " \
                "Microsoft version, set _CRT_NON_CONFORMING_SWPRINTFS.")
    #else
        #define _SWPRINTFS_DEPRECATED
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _ACRTIMP int __cdecl __stdio_common_vswprintf(
        _In_                                    unsigned __int64 _Options,
        _Out_writes_opt_z_(_BufferCount)        wchar_t*         _Buffer,
        _In_                                    size_t           _BufferCount,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Success_(return >= 0)
    _Check_return_opt_
    _ACRTIMP int __cdecl __stdio_common_vswprintf_s(
        _In_                                    unsigned __int64 _Options,
        _Out_writes_z_(_BufferCount)            wchar_t*         _Buffer,
        _In_                                    size_t           _BufferCount,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Success_(return >= 0)
    _Check_return_opt_
    _ACRTIMP int __cdecl __stdio_common_vsnwprintf_s(
        _In_                                    unsigned __int64 _Options,
        _Out_writes_opt_z_(_BufferCount)        wchar_t*         _Buffer,
        _In_                                    size_t           _BufferCount,
        _In_                                    size_t           _MaxCount,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Success_(return >= 0)
    _Check_return_opt_
    _ACRTIMP int __cdecl __stdio_common_vswprintf_p(
        _In_                                    unsigned __int64 _Options,
        _Out_writes_z_(_BufferCount)            wchar_t*         _Buffer,
        _In_                                    size_t           _BufferCount,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vsnwprintf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _vsnwprintf_l(
        _Out_writes_opt_(_BufferCount) _Post_maybez_ wchar_t*       const _Buffer,
        _In_                                         size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(2)      wchar_t const* const _Format,
        _In_opt_                                     _locale_t      const _Locale,
                                                     va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vswprintf(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsnwprintf_s_l(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_                                              size_t         const _MaxCount,
        _In_z_ _Printf_format_string_params_(2)           wchar_t const* const _Format,
        _In_opt_                                          _locale_t      const _Locale,
                                                          va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vsnwprintf_s(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Buffer, _BufferCount, _MaxCount, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsnwprintf_s(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_                                              size_t         const _MaxCount,
        _In_z_ _Printf_format_string_                     wchar_t const* const _Format,
                                                          va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vsnwprintf_s_l(_Buffer, _BufferCount, _MaxCount, _Format, NULL, _ArgList);
    }
    #endif

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST_EX(
        _Success_(return >= 0)
        int, __RETURN_POLICY_SAME, _CRT_STDIO_INLINE, __CRTDECL, _snwprintf, _vsnwprintf,
        _Pre_notnull_ _Post_maybez_                   wchar_t,
        _Out_writes_opt_(_BufferCount) _Post_maybez_, wchar_t,        _Buffer,
        _In_                                          size_t,         _BufferCount,
        _In_z_ _Printf_format_string_                 wchar_t const*, _Format
        )

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vsnwprintf_s)
    _CRT_STDIO_INLINE int __CRTDECL _vsnwprintf(
        _Out_writes_opt_(_BufferCount) _Post_maybez_ wchar_t*       _Buffer,
        _In_                                         size_t         _BufferCount,
        _In_z_ _Printf_format_string_                wchar_t const* _Format,
                                                     va_list        _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vsnwprintf_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
    }
    #endif

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(
        _Success_(return >= 0)
        int, _vsnwprintf_s,
        _Always_(_Post_z_)            wchar_t,        _Buffer,
        _In_                          size_t,         _BufferCount,
        _In_z_ _Printf_format_string_ wchar_t const*, _Format,
                                      va_list,        _ArgList
        )

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vswprintf_c_l(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(2)           wchar_t const* const _Format,
        _In_opt_                                          _locale_t      const _Locale,
                                                          va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vswprintf(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vswprintf_c(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_z_ _Printf_format_string_                     wchar_t const* const _Format,
                                                          va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vswprintf_c_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vswprintf_l(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(2)           wchar_t const* const _Format,
        _In_opt_                                          _locale_t      const _Locale,
                                                          va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vswprintf_c_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL __vswprintf_l(
        _Pre_notnull_ _Always_(_Post_z_)        wchar_t*       const _Buffer,
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vswprintf_l(_Buffer, (size_t)-1, _Format, _Locale, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vswprintf(
        _Pre_notnull_ _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_z_ _Printf_format_string_    wchar_t const* const _Format,
                                         va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vswprintf_l(_Buffer, (size_t)-1, _Format, NULL, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vswprintf(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(1)           wchar_t const* const _Format,
                                                          va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vswprintf_c_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vswprintf_s_l(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                          size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(2)       wchar_t const* const _Format,
        _In_opt_                                      _locale_t      const _Locale,
                                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vswprintf_s(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Success_(return >= 0)
        _CRT_STDIO_INLINE int __CRTDECL vswprintf_s(
            _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
            _In_                                          size_t         const _BufferCount,
            _In_z_ _Printf_format_string_                 wchar_t const* const _Format,
                                                          va_list              _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vswprintf_s_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
        }
    #endif

    #endif

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        _Success_(return >= 0)
        int, vswprintf_s,
        _Always_(_Post_z_)            wchar_t,        _Buffer,
        _In_z_ _Printf_format_string_ wchar_t const*, _Format,
                                      va_list,        _ArgList
        )

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vswprintf_p_l(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                          size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(2)       wchar_t const* const _Format,
        _In_opt_                                      _locale_t      const _Locale,
                                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vswprintf_p(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vswprintf_p(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                          size_t         const _BufferCount,
        _In_z_ _Printf_format_string_                 wchar_t const* const _Format,
                                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vswprintf_p_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _vscwprintf_l(
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vswprintf(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
            NULL, 0, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _vscwprintf(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vscwprintf_l(_Format, NULL, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _vscwprintf_p_l(
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vswprintf_p(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
            NULL, 0, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _vscwprintf_p(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vscwprintf_p_l(_Format, NULL, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL __swprintf_l(
        _Pre_notnull_ _Always_(_Post_z_)        wchar_t*       const _Buffer,
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = __vswprintf_l(_Buffer, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _swprintf_l(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(0)           wchar_t const* const _Format,
        _In_opt_                                          _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vswprintf_c_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _swprintf(
        _Pre_notnull_ _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_z_ _Printf_format_string_    wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = __vswprintf_l(_Buffer, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL swprintf(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_z_ _Printf_format_string_                     wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vswprintf_c_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_2_ARGLIST_EX(
        _Success_(return >= 0)
        int, __RETURN_POLICY_SAME, _CRT_STDIO_INLINE, __CRTDECL, __swprintf_l, __vswprintf_l, _vswprintf_s_l,
        _Pre_notnull_ _Always_(_Post_z_)        wchar_t,
        _Pre_notnull_ _Always_(_Post_z_),       wchar_t,        _Buffer,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*, _Format,
        _In_opt_                                _locale_t,      _Locale
        )

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST_EX(
        _Success_(return >= 0)
        int, __RETURN_POLICY_SAME, _CRT_STDIO_INLINE, __CRTDECL, _swprintf, swprintf_s, _vswprintf, vswprintf_s,
        _Pre_notnull_ _Always_(_Post_z_), wchar_t,        _Buffer,
        _In_z_ _Printf_format_string_     wchar_t const*, _Format
        )

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _swprintf_s_l(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                          size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(0)       wchar_t const* const _Format,
        _In_opt_                                      _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vswprintf_s_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Success_(return >= 0)
        _CRT_STDIO_INLINE int __CRTDECL swprintf_s(
            _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
            _In_                                          size_t         const _BufferCount,
            _In_z_ _Printf_format_string_                 wchar_t const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vswprintf_s_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1_ARGLIST(
        _Success_(return >= 0)
        int, swprintf_s, vswprintf_s,
        _Always_(_Post_z_)            wchar_t,        _Buffer,
        _In_z_ _Printf_format_string_ wchar_t const*, _Format
        )

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _swprintf_p_l(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                          size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(0)       wchar_t const* const _Format,
        _In_opt_                                      _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vswprintf_p_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _swprintf_p(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                          size_t         const _BufferCount,
        _In_z_ _Printf_format_string_                 wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vswprintf_p_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _swprintf_c_l(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(0)           wchar_t const* const _Format,
        _In_opt_                                          _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vswprintf_c_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _swprintf_c(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_z_ _Printf_format_string_                     wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vswprintf_c_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snwprintf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _snwprintf_l(
        _Out_writes_opt_(_BufferCount) _Post_maybez_ wchar_t*       const _Buffer,
        _In_                                         size_t         const _BufferCount,
        _In_z_ _Printf_format_string_params_(0)      wchar_t const* const _Format,
        _In_opt_                                     _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);

        _Result = _vsnwprintf_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snwprintf(
        _Out_writes_opt_(_BufferCount) _Post_maybez_ wchar_t*       _Buffer,
        _In_                                         size_t         _BufferCount,
        _In_z_ _Printf_format_string_                wchar_t const* _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);

        _Result = _vsnwprintf_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snwprintf_s_l(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_                                              size_t         const _MaxCount,
        _In_z_ _Printf_format_string_params_(0)           wchar_t const* const _Format,
        _In_opt_                                          _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vsnwprintf_s_l(_Buffer, _BufferCount, _MaxCount, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snwprintf_s(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t*       const _Buffer,
        _In_                                              size_t         const _BufferCount,
        _In_                                              size_t         const _MaxCount,
        _In_z_ _Printf_format_string_                     wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vsnwprintf_s_l(_Buffer, _BufferCount, _MaxCount, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2_ARGLIST(
        _Success_(return >= 0)
        int, _snwprintf_s, _vsnwprintf_s,
        _Always_(_Post_z_)            wchar_t,        _Buffer,
        _In_                          size_t,         _BufferCount,
        _In_z_ _Printf_format_string_ wchar_t const*, _Format
        )

    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _scwprintf_l(
        _In_z_ _Printf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vscwprintf_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _scwprintf(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vscwprintf_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _scwprintf_p_l(
        _In_z_ _Printf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vscwprintf_p_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _scwprintf_p(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vscwprintf_p_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif


    #if !defined RC_INVOKED && !defined __midl && !defined _INC_SWPRINTF_INL_
        // C4141: double deprecation
        // C6054: string may not be zero-terminated
        #pragma warning(push)
        #pragma warning(disable: 4141 6054)

        #ifdef __cplusplus

            extern "C++" _SWPRINTFS_DEPRECATED _CRT_INSECURE_DEPRECATE(swprintf_s)
            inline int swprintf(
                _Pre_notnull_ _Post_z_        wchar_t*       const _Buffer,
                _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                ...) throw()
            {
                int _Result;
                va_list _ArgList;
                __crt_va_start(_ArgList, _Format);
                _Result = vswprintf(_Buffer, _CRT_INT_MAX, _Format, _ArgList);
                __crt_va_end(_ArgList);
                return _Result;
            }

            extern "C++" _SWPRINTFS_DEPRECATED _CRT_INSECURE_DEPRECATE(vswprintf_s)
            inline int __CRTDECL vswprintf(
                _Pre_notnull_ _Post_z_        wchar_t*       const _Buffer,
                _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                              va_list              _ArgList
                ) throw()
            {
                return vswprintf(_Buffer, _CRT_INT_MAX, _Format, _ArgList);
            }

            extern "C++" _SWPRINTFS_DEPRECATED _CRT_INSECURE_DEPRECATE(_swprintf_s_l)
            inline int _swprintf_l(
                _Pre_notnull_ _Post_z_                  wchar_t*       const _Buffer,
                _In_z_ _Printf_format_string_params_(0) wchar_t const* const _Format,
                _In_opt_                                _locale_t      const _Locale,
                ...) throw()
            {
                int _Result;
                va_list _ArgList;
                __crt_va_start(_ArgList, _Locale);
                _Result = _vswprintf_l(_Buffer, (size_t)-1, _Format, _Locale, _ArgList);
                __crt_va_end(_ArgList);
                return _Result;
            }

            extern "C++" _SWPRINTFS_DEPRECATED _CRT_INSECURE_DEPRECATE(_vswprintf_s_l)
            inline int __CRTDECL _vswprintf_l(
                _Pre_notnull_ _Post_z_                  wchar_t*       const _Buffer,
                _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
                _In_opt_                                _locale_t      const _Locale,
                                                        va_list              _ArgList
                ) throw()
            {
                return _vswprintf_l(_Buffer, (size_t)-1, _Format, _Locale, _ArgList);
            }

        #endif  // __cplusplus

        #pragma warning(pop)
    #endif  // !_INC_SWPRINTF_INL_

    #if defined _CRT_NON_CONFORMING_SWPRINTFS && !defined __cplusplus
        #define swprintf     _swprintf
        #define vswprintf    _vswprintf
        #define _swprintf_l  __swprintf_l
        #define _vswprintf_l __vswprintf_l
    #endif


    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Wide Character Formatted Input Functions (String)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Success_(return >= 0)
    _ACRTIMP int __cdecl __stdio_common_vswscanf(
        _In_                                   unsigned __int64 _Options,
        _In_reads_(_BufferCount) _Pre_z_       wchar_t const*   _Buffer,
        _In_                                   size_t           _BufferCount,
        _In_z_ _Scanf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                               _locale_t        _Locale,
                                               va_list          _ArgList
        );

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vswscanf_l(
        _In_z_                        wchar_t const* const _Buffer,
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        _In_opt_                      _locale_t      const _Locale,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vswscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS,
            _Buffer, (size_t)-1, _Format, _Locale, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vswscanf(
        _In_z_                        wchar_t const* _Buffer,
        _In_z_ _Printf_format_string_ wchar_t const* _Format,
                                      va_list        _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vswscanf_l(_Buffer, _Format, NULL, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vswscanf_s_l(
        _In_z_                        wchar_t const* const _Buffer,
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        _In_opt_                      _locale_t      const _Locale,
                                      va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vswscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT,
            _Buffer, (size_t)-1, _Format, _Locale, _ArgList);
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Success_(return >= 0)
        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vswscanf_s(
            _In_z_                        wchar_t const* const _Buffer,
            _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                          va_list              _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vswscanf_s_l(_Buffer, _Format, NULL, _ArgList);
        }
    #endif

    #endif

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
        _Success_(return >= 0)
        int, vswscanf_s,
        _In_z_                        wchar_t,        _Buffer,
        _In_z_ _Printf_format_string_ wchar_t const*, _Format,
                                      va_list,        _ArgList
        )

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vsnwscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _vsnwscanf_l(
        _In_reads_(_BufferCount) _Pre_z_       wchar_t const* const _Buffer,
        _In_                                   size_t         const _BufferCount,
        _In_z_ _Scanf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                               _locale_t      const _Locale,
                                               va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vswscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsnwscanf_s_l(
        _In_reads_(_BufferCount) _Pre_z_         wchar_t const* const _Buffer,
        _In_                                     size_t         const _BufferCount,
        _In_z_ _Scanf_s_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                 _locale_t      const _Locale,
                                                 va_list              _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vswscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_swscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _swscanf_l(
        _In_z_                                 wchar_t const* const _Buffer,
        _In_z_ _Scanf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                               _locale_t            _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vswscanf_l(_Buffer, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_ _CRT_INSECURE_DEPRECATE(swscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL swscanf(
        _In_z_                       wchar_t const* const _Buffer,
        _In_z_ _Scanf_format_string_ wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vswscanf_l(_Buffer, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _swscanf_s_l(
        _In_z_                                   wchar_t const* const _Buffer,
        _In_z_ _Scanf_s_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                 _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vswscanf_s_l(_Buffer, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Success_(return >= 0)
        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL swscanf_s(
            _In_z_                         wchar_t const* const _Buffer,
            _In_z_ _Scanf_s_format_string_ wchar_t const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vswscanf_s_l(_Buffer, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snwscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _snwscanf_l(
        _In_reads_(_BufferCount) _Pre_z_       wchar_t const* const _Buffer,
        _In_                                   size_t         const _BufferCount,
        _In_z_ _Scanf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                               _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);

        _Result = _vsnwscanf_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snwscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL _snwscanf(
        _In_reads_(_BufferCount) _Pre_z_ wchar_t const* const _Buffer,
        _In_                             size_t         const _BufferCount,
        _In_z_ _Scanf_format_string_     wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);

        _Result = _vsnwscanf_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snwscanf_s_l(
        _In_reads_(_BufferCount) _Pre_z_         wchar_t const* const _Buffer,
        _In_                                     size_t         const _BufferCount,
        _In_z_ _Scanf_s_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                                 _locale_t      const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vsnwscanf_s_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snwscanf_s(
        _In_reads_(_BufferCount) _Pre_z_  wchar_t const* const _Buffer,
        _In_                              size_t         const _BufferCount,
        _In_z_ _Scanf_s_format_string_    wchar_t const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vsnwscanf_s_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #ifdef _M_CEE_MIXED
        #pragma managed(pop)
    #endif
#endif // _CRT_FUNCTIONS_REQUIRED

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
