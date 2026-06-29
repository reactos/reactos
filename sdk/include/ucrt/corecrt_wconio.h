//
// corecrt_wconio.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the wide character (wchar_t) console I/O functionality,
// shared by both <conio.h> and <wchar.h>
//
#pragma once

#include <corecrt.h>
#include <corecrt_stdio_config.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

#define WEOF ((wint_t)(0xFFFF))



#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

    _Check_return_wat_
    _Success_(_BufferCount > 0)
    _DCRTIMP errno_t __cdecl _cgetws_s(
        _Out_writes_to_(_BufferCount, *_SizeRead) wchar_t* _Buffer,
        _In_                                      size_t   _BufferCount,
        _Out_                                     size_t*  _SizeRead
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
        _Success_(return == 0)
        errno_t, _cgetws_s,
        _Out_writes_z_(*_Buffer) wchar_t, _Buffer,
        _In_                     size_t*, _SizeRead
        )

    _Check_return_opt_
    _DCRTIMP int __cdecl _cputws(
        _In_z_ wchar_t const* _Buffer
        );

    _Check_return_     _DCRTIMP wint_t __cdecl _getwch  (void);
    _Check_return_     _DCRTIMP wint_t __cdecl _getwche (void);
    _Check_return_opt_ _DCRTIMP wint_t __cdecl _putwch  (_In_ wchar_t _Character);
    _Check_return_opt_ _DCRTIMP wint_t __cdecl _ungetwch(_In_ wint_t  _Character);

    _Check_return_     _DCRTIMP wint_t __cdecl _getwch_nolock  (void);
    _Check_return_     _DCRTIMP wint_t __cdecl _getwche_nolock (void);
    _Check_return_opt_ _DCRTIMP wint_t __cdecl _putwch_nolock  (_In_ wchar_t _Character);
    _Check_return_opt_ _DCRTIMP wint_t __cdecl _ungetwch_nolock(_In_ wint_t  _Character);



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Wide Character Formatted Output Functions (Console)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_opt_
    _DCRTIMP int __cdecl __conio_common_vcwprintf(
        _In_                                    unsigned __int64 _Options,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Check_return_opt_
    _DCRTIMP int __cdecl __conio_common_vcwprintf_s(
        _In_                                    unsigned __int64 _Options,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Check_return_opt_
    _DCRTIMP int __cdecl __conio_common_vcwprintf_p(
        _In_                                    unsigned __int64 _Options,
        _In_z_ _Printf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcwprintf_l(
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return __conio_common_vcwprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, _Format, _Locale, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcwprintf(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                      va_list              _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return _vcwprintf_l(_Format, NULL, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcwprintf_s_l(
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return __conio_common_vcwprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, _Format, _Locale, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcwprintf_s(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
                                      va_list              _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return _vcwprintf_s_l(_Format, NULL, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcwprintf_p_l(
        _In_z_ _Printf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                                _locale_t      const _Locale,
                                                va_list              _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return __conio_common_vcwprintf_p(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, _Format, _Locale, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcwprintf_p(
        _In_z_ _Printf_format_string_ const wchar_t* const _Format,
                                      va_list              _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return _vcwprintf_p_l(_Format, NULL, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cwprintf_l(
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
        _Result = _vcwprintf_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cwprintf(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vcwprintf_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cwprintf_s_l(
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
        _Result = _vcwprintf_s_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cwprintf_s(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vcwprintf_s_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cwprintf_p_l(
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
        _Result = _vcwprintf_p_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cwprintf_p(
        _In_z_ _Printf_format_string_ wchar_t const* const _Format,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vcwprintf_p_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Wide Character Formatted Input Functions (Console)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_opt_
    _DCRTIMP int __cdecl __conio_common_vcwscanf(
        _In_                                   unsigned __int64 _Options,
        _In_z_ _Scanf_format_string_params_(2) wchar_t const*   _Format,
        _In_opt_                               _locale_t        _Locale,
                                               va_list          _ArgList
        );

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vcwscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _vcwscanf_l(
        _In_z_ _Scanf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                               _locale_t      const _Locale,
                                               va_list              _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return __conio_common_vcwscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS,
            _Format, _Locale, _ArgList);
    }
#endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vcwscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL _vcwscanf(
        _In_z_ _Scanf_format_string_params_(1) wchar_t const* const _Format,
                                               va_list              _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return _vcwscanf_l(_Format, NULL, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcwscanf_s_l(
        _In_z_ _Scanf_format_string_params_(2) wchar_t const* const _Format,
        _In_opt_                               _locale_t      const _Locale,
                                               va_list              _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return __conio_common_vcwscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT,
            _Format, _Locale, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcwscanf_s(
        _In_z_ _Scanf_format_string_params_(1) wchar_t const* const _Format,
                                               va_list              _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return _vcwscanf_s_l(_Format, NULL, _ArgList);
    }
#endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_cwscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _cwscanf_l(
        _In_z_ _Scanf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                               _locale_t      const _Locale,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);

        _Result = _vcwscanf_l(_Format, _Locale, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_cwscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL _cwscanf(
        _In_z_ _Scanf_format_string_ wchar_t const* const _Format,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);

        _Result = _vcwscanf_l(_Format, NULL, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cwscanf_s_l(
        _In_z_ _Scanf_format_string_params_(0) wchar_t const* const _Format,
        _In_opt_                               _locale_t      const _Locale,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vcwscanf_s_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cwscanf_s(
        _In_z_ _Scanf_format_string_ wchar_t const* const _Format,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vcwscanf_s_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

#endif // _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
