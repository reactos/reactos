//
// locale.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The C locale library.
//
#pragma once
#ifndef _INC_LOCALE // include guard for 3rd party interop
#define _INC_LOCALE

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



// Locale categories
#define LC_ALL          0
#define LC_COLLATE      1
#define LC_CTYPE        2
#define LC_MONETARY     3
#define LC_NUMERIC      4
#define LC_TIME         5

#define LC_MIN          LC_ALL
#define LC_MAX          LC_TIME

// Locale convention structure
struct lconv
{
    char*    decimal_point;
    char*    thousands_sep;
    char*    grouping;
    char*    int_curr_symbol;
    char*    currency_symbol;
    char*    mon_decimal_point;
    char*    mon_thousands_sep;
    char*    mon_grouping;
    char*    positive_sign;
    char*    negative_sign;
    char     int_frac_digits;
    char     frac_digits;
    char     p_cs_precedes;
    char     p_sep_by_space;
    char     n_cs_precedes;
    char     n_sep_by_space;
    char     p_sign_posn;
    char     n_sign_posn;
    wchar_t* _W_decimal_point;
    wchar_t* _W_thousands_sep;
    wchar_t* _W_int_curr_symbol;
    wchar_t* _W_currency_symbol;
    wchar_t* _W_mon_decimal_point;
    wchar_t* _W_mon_thousands_sep;
    wchar_t* _W_positive_sign;
    wchar_t* _W_negative_sign;
};

struct tm;

// ANSI: char lconv members default is CHAR_MAX which is compile time dependent.
// Defining and using __do_unsigned_char_lconv_initialization here causes CRT
// startup code to initialize lconv members properly
#ifdef _CHAR_UNSIGNED
    extern int __do_unsigned_char_lconv_initialization;
    extern __inline int __use_char_max(void)
    {
        return __do_unsigned_char_lconv_initialization;
    }
#endif



#define _ENABLE_PER_THREAD_LOCALE         0x0001
#define _DISABLE_PER_THREAD_LOCALE        0x0002
#define _ENABLE_PER_THREAD_LOCALE_GLOBAL  0x0010
#define _DISABLE_PER_THREAD_LOCALE_GLOBAL 0x0020
#define _ENABLE_PER_THREAD_LOCALE_NEW     0x0100
#define _DISABLE_PER_THREAD_LOCALE_NEW    0x0200

#if _CRT_FUNCTIONS_REQUIRED

    _ACRTIMP void __cdecl _lock_locales(void);
    _ACRTIMP void __cdecl _unlock_locales(void);

    _Check_return_opt_
    _ACRTIMP int __cdecl _configthreadlocale(
        _In_ int _Flag
        );

    _Check_return_opt_ _Success_(return != 0) _Ret_z_
    _ACRTIMP char* __cdecl setlocale(
        _In_       int         _Category,
        _In_opt_z_ char const* _Locale
        );

    _Check_return_opt_
    _ACRTIMP struct lconv* __cdecl localeconv(void);

    _Check_return_opt_
    _ACRTIMP _locale_t __cdecl _get_current_locale(void);

    _Check_return_opt_
    _ACRTIMP _locale_t __cdecl _create_locale(
        _In_   int         _Category,
        _In_z_ char const* _Locale
        );

    _ACRTIMP void __cdecl _free_locale(
        _In_opt_ _locale_t _Locale
        );

    // Also declared in <wchar.h>
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



    _ACRTIMP wchar_t**    __cdecl ___lc_locale_name_func(void);
    _ACRTIMP unsigned int __cdecl ___lc_codepage_func   (void);
    _ACRTIMP unsigned int __cdecl ___lc_collate_cp_func (void);




    // Time-related functions
    _Success_(return != 0)
    _Ret_z_
    _ACRTIMP char*    __cdecl _Getdays(void);

    _Success_(return != 0)
    _Ret_z_
    _ACRTIMP char*    __cdecl _Getmonths(void);

    _ACRTIMP void*    __cdecl _Gettnames(void);

    _Success_(return != 0)
    _Ret_z_
    _ACRTIMP wchar_t* __cdecl _W_Getdays(void);

    _Success_(return != 0)
    _Ret_z_
    _ACRTIMP wchar_t* __cdecl _W_Getmonths(void);

    _ACRTIMP void*    __cdecl _W_Gettnames(void);

    _Success_(return > 0)
    _ACRTIMP size_t __cdecl _Strftime(
        _Out_writes_z_(_Max_size) char*           _Buffer,
        _In_                     size_t           _Max_size,
        _In_z_                   char const*      _Format,
        _In_                     struct tm const* _Timeptr,
        _In_opt_                 void*            _Lc_time_arg);

    _Success_(return > 0)
    _ACRTIMP size_t __cdecl _Wcsftime(
        _Out_writes_z_(_Max_size) wchar_t*        _Buffer,
        _In_                     size_t           _Max_size,
        _In_z_                   wchar_t const*   _Format,
        _In_                     struct tm const* _Timeptr,
        _In_opt_                 void*            _Lc_time_arg
        );

#endif // _CRT_FUNCTIONS_REQUIRED


_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_LOCALE
