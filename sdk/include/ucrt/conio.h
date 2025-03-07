//
// conio.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the direct console I/O functions.
//
#pragma once
#ifndef _INC_CONIO // include guard for 3rd party interop
#define _INC_CONIO

#include <corecrt.h>
#include <corecrt_wconio.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

    _Check_return_wat_
    _Success_(_BufferCount > 0)
    _DCRTIMP errno_t __cdecl _cgets_s(
        _Out_writes_z_(_BufferCount) char*   _Buffer,
        _In_                         size_t  _BufferCount,
        _Out_                        size_t* _SizeRead
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(
        _Success_(return == 0)
        errno_t, _cgets_s,
        _Out_writes_z_(*_Buffer) char,    _Buffer,
        _Out_                    size_t*, _SizeRead
        )

    _Check_return_opt_
    _DCRTIMP int __cdecl _cputs(
        _In_z_ char const* _Buffer
        );

    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Narrow Character Formatted Output Functions (Console)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_opt_
    _DCRTIMP int __cdecl __conio_common_vcprintf(
        _In_                                    unsigned __int64 _Options,
        _In_z_ _Printf_format_string_params_(2) char const*      _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Check_return_opt_
    _DCRTIMP int __cdecl __conio_common_vcprintf_s(
        _In_                                    unsigned __int64 _Options,
        _In_z_ _Printf_format_string_params_(2) char const*      _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Check_return_opt_
    _DCRTIMP int __cdecl __conio_common_vcprintf_p(
        _In_                                    unsigned __int64 _Options,
        _In_z_ _Printf_format_string_params_(2) char const*      _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcprintf_l(
        _In_z_ _Printf_format_string_params_(2) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
                                                va_list           _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return __conio_common_vcprintf(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Format, _Locale, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcprintf(
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return _vcprintf_l(_Format, NULL, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcprintf_s_l(
        _In_z_ _Printf_format_string_params_(2) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
                                                va_list           _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return __conio_common_vcprintf_s(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Format, _Locale, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcprintf_s(
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return _vcprintf_s_l(_Format, NULL, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcprintf_p_l(
        _In_z_ _Printf_format_string_params_(2) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
                                                va_list           _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return __conio_common_vcprintf_p(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Format, _Locale, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcprintf_p(
        _In_z_ char const* const _Format,
               va_list           _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return _vcprintf_p_l(_Format, NULL, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cprintf_l(
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vcprintf_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cprintf(
        _In_z_ _Printf_format_string_ char const* const _Format,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vcprintf_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cprintf_s_l(
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vcprintf_s_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cprintf_s(
        _In_z_ _Printf_format_string_ char const* const _Format,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vcprintf_s_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cprintf_p_l(
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vcprintf_p_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cprintf_p(
        _In_z_ _Printf_format_string_ char const* const _Format,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vcprintf_p_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif


    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Narrow Character Formatted Input Functions (Console)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Check_return_opt_
    _DCRTIMP int __cdecl __conio_common_vcscanf(
        _In_                                   unsigned __int64 _Options,
        _In_z_ _Scanf_format_string_params_(2) char const*      _Format,
        _In_opt_                               _locale_t        _Locale,
                                               va_list          _ArgList
        );

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vcscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _vcscanf_l(
        _In_z_ _Scanf_format_string_params_(2) char const* const _Format,
        _In_opt_                               _locale_t   const _Locale,
                                               va_list           _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return __conio_common_vcscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS,
            _Format, _Locale, _ArgList);
    }
#endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vcscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL _vcscanf(
        _In_z_ _Scanf_format_string_params_(1) char const* const _Format,
                                               va_list           _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return _vcscanf_l(_Format, NULL, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcscanf_s_l(
        _In_z_ _Scanf_format_string_params_(2) char const* const _Format,
        _In_opt_                               _locale_t   const _Locale,
                                               va_list           _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return __conio_common_vcscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT,
            _Format, _Locale, _ArgList);
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vcscanf_s(
        _In_z_ _Scanf_format_string_params_(1) char const* const _Format,
                                               va_list           _ArgList
        )
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        return _vcscanf_s_l(_Format, NULL, _ArgList);
    }
#endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_cscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _cscanf_l(
        _In_z_ _Scanf_format_string_params_(0) char const* const _Format,
        _In_opt_                               _locale_t   const _Locale,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);

        _Result = _vcscanf_l(_Format, _Locale, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_cscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL _cscanf(
        _In_z_ _Scanf_format_string_ char const* const _Format,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);

        _Result = _vcscanf_l(_Format, NULL, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cscanf_s_l(
        _In_z_ _Scanf_format_string_params_(0) char const* const _Format,
        _In_opt_                               _locale_t   const _Locale,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vcscanf_s_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _cscanf_s(
        _In_z_ _Scanf_format_string_ char const* const _Format,
        ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vcscanf_s_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
#endif


    _DCRTIMP int __cdecl _kbhit(void);

    _Check_return_     _DCRTIMP int __cdecl _getch(void);
    _Check_return_     _DCRTIMP int __cdecl _getche(void);
    _Check_return_opt_ _DCRTIMP int __cdecl _putch  (_In_ int _Ch);
    _Check_return_opt_ _DCRTIMP int __cdecl _ungetch(_In_ int _Ch);

    _Check_return_     _DCRTIMP int __cdecl _getch_nolock  (void);
    _Check_return_     _DCRTIMP int __cdecl _getche_nolock (void);
    _Check_return_opt_ _DCRTIMP int __cdecl _putch_nolock  (_In_ int _Ch);
    _Check_return_opt_ _DCRTIMP int __cdecl _ungetch_nolock(_In_ int _Ch);

    #if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES

        // Suppress double-deprecation warnings:
        #pragma warning(push)
        #pragma warning(disable: 4141)

        _Success_(return != 0)
        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_cgets) _CRT_INSECURE_DEPRECATE(_cgets_s)
        _DCRTIMP char* __cdecl cgets(
            _At_(&_Buffer[0], _In_reads_(1))
            _At_(&_Buffer[1], _Out_writes_(1))
            _At_(&_Buffer[2], _Post_z_ _Out_writes_to_(_Buffer[0], _Buffer[1]))
            char* _Buffer
            );

        #pragma warning(pop)

        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_cputs)
        _DCRTIMP int __cdecl cputs(
            _In_z_ char const* _String
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_getch)
        _DCRTIMP int __cdecl getch(void);

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_getche)
        _DCRTIMP int __cdecl getche(void);

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_kbhit)
        _DCRTIMP int __cdecl kbhit(void);

        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_putch)
        _DCRTIMP int __cdecl putch(
            _In_ int _Ch
            );

        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_ungetch)
        _DCRTIMP int __cdecl ungetch(
            _In_ int _Ch
            );

        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_cprintf)
        _CRT_STDIO_INLINE int __CRTDECL cprintf(
            _In_z_ _Printf_format_string_ char const* const _Format,
            ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vcprintf_l(_Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
#endif

        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_cscanf)
        _CRT_STDIO_INLINE int __CRTDECL cscanf(
            _In_z_ _Scanf_format_string_ char const* const _Format,
            ...)
#if defined _NO_CRT_STDIO_INLINE
;
#else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);

            _Result = _vcscanf_l(_Format, NULL, _ArgList);

            __crt_va_end(_ArgList);
            return _Result;
        }
#endif

    #endif // _CRT_INTERNAL_NONSTDC_NAMES

_CRT_END_C_HEADER

_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_CONIO
