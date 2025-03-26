//
// _ctype.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the function equivalents of the character classification macros in
// <ctype.h>.  These function definitions make use of the macros.  The functions
// return zero if the character does not meet the requirements, and nonzero if
// the character does meet the requirements.
//
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>

// The ctype functions (isalnum(), isupper(), etc) are very small and quick.
// Optimizing for size has a measurable negative impact, so we optimize for speed here.
#pragma optimize("t", on)

static __forceinline int fast_check_initial_locale(int const c, int const mask) throw()
{
    #ifdef _DEBUG
    return _chvalidator(c, mask);
    #else
    return __acrt_locale_get_ctype_array_value(__acrt_initial_locale_data._public._locale_pctype, c, mask);
    #endif
}

static __forceinline int fast_check_current_locale(int const c, int const mask) throw()
{
    // Avoid PTD lookup when locale is unchanged.
    if (!__acrt_locale_changed())
    {
        return fast_check_initial_locale(c, mask);
    }

    // Avoid _LocaleUpdate overhead:
    // * Extra un-inlined function calls
    // * Multibyte locale synchronization
    // * Marking/unmarking current thread as using per-thread-locales.

    __acrt_ptd * const ptd{__acrt_getptd()};
    __crt_locale_data * locale_info{ptd->_locale_info};
    __acrt_update_locale_info(ptd, &locale_info);

    // Common case
    if (c >= -1 && c <= 255)
    {
        return locale_info->_public._locale_pctype[c] & mask;
    }

    // Microsoft Extension: Translate int values outside of unsigned char range for DBCS locales
    // Note that our documentation also clearly states this is undefined.
    if (locale_info->_public._locale_mb_cur_max > 1)
    {
        return _isctype_l(c, mask, nullptr);
    }

    return 0;
}

static __forceinline int fast_check_given_locale(int const c, int const mask, _locale_t const locale) throw()
{
    // Avoid _LocaleUpdate overhead - just check whether it's nullptr.
    if (locale == nullptr) {
        return fast_check_current_locale(c, mask);
    }

    // Common case
    if (c >= -1 && c <= 255)
    {
        return locale->locinfo->_public._locale_pctype[c] & mask;
    }

    // Microsoft Extension: Translate int values outside of unsigned char range for DBCS locales
    // Note that our documentation also clearly states this is undefined.
    if (locale->locinfo->_public._locale_mb_cur_max > 1)
    {
        return _isctype_l(c, mask, locale);
    }

    return 0;
}


extern "C" int (__cdecl _isalpha_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _ALPHA, locale);
}

extern "C" int (__cdecl isalpha)(int const c)
{
    return fast_check_current_locale(c, _ALPHA);
}


extern "C" int (__cdecl _isupper_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _UPPER, locale);
}

extern "C" int (__cdecl isupper)(int const c)
{
    return fast_check_current_locale(c, _UPPER);
}


extern "C" int (__cdecl _islower_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _LOWER, locale);
}

extern "C" int (__cdecl islower)(int const c)
{
    return fast_check_current_locale(c, _LOWER);
}


extern "C" int (__cdecl _isdigit_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _DIGIT, locale);
}

extern "C" int (__cdecl isdigit)(int const c)
{
    return fast_check_current_locale(c, _DIGIT);
}


extern "C" int (__cdecl _isxdigit_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _HEX, locale);
}

extern "C" int (__cdecl isxdigit)(int const c)
{
    return fast_check_current_locale(c, _HEX);
}


extern "C" int (__cdecl _isspace_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _SPACE, locale);
}

extern "C" int (__cdecl isspace)(int const c)
{
    return fast_check_current_locale(c, _SPACE);
}


extern "C" int (__cdecl _ispunct_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _PUNCT, locale);
}

extern "C" int (__cdecl ispunct)(int const c)
{
    return fast_check_current_locale(c, _PUNCT);
}


extern "C" int (__cdecl _isblank_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _BLANK, locale);
}

extern "C" int (__cdecl isblank)(int const c)
{
    // \t is a blank character, but is not registered as _Blank on the table, because that will make it
    //printable. Also Windows (via GetStringType()) considered all _BLANK characters to also be _PRINT characters,
    //so does not have a way to specify blank, non-printable.
    if (c == '\t') {
        return _BLANK;
    }

    return fast_check_current_locale(c, _BLANK);
}


extern "C" int (__cdecl _isalnum_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _ALPHA | _DIGIT, locale);
}

extern "C" int (__cdecl isalnum)(int const c)
{
    return fast_check_current_locale(c, _ALPHA | _DIGIT);
}


extern "C" int (__cdecl _isprint_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _BLANK | _PUNCT | _ALPHA | _DIGIT, locale);
}

extern "C" int (__cdecl isprint)(int const c)
{
    return fast_check_current_locale(c, _BLANK | _PUNCT | _ALPHA | _DIGIT);
}


extern "C" int (__cdecl _isgraph_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _PUNCT | _ALPHA | _DIGIT, locale);
}

extern "C" int (__cdecl isgraph)(int const c)
{
    return fast_check_current_locale(c, _PUNCT | _ALPHA | _DIGIT);
}


extern "C" int (__cdecl _iscntrl_l)(int const c, _locale_t const locale)
{
    return fast_check_given_locale(c, _CONTROL, locale);
}

extern "C" int (__cdecl iscntrl)(int const c)
{
    return fast_check_current_locale(c, _CONTROL);
}


extern "C" int (__cdecl __isascii)(int const c)
{
    return __isascii(c);
}

extern "C" int (__cdecl __toascii)(int const c)
{
    return __toascii(c);
}


extern "C" int (__cdecl _iscsymf_l)(int const c, _locale_t const locale)
{
    return (_isalpha_l)(c, locale) || c == '_';
}

extern "C" int (__cdecl __iscsymf)(int const c)
{
    return __iscsymf(c);
}


extern "C" int (__cdecl _iscsym_l)(int const c, _locale_t const locale)
{
    return (_isalnum_l)(c, locale) || c == '_';
}

extern "C" int (__cdecl __iscsym)(int const c)
{
    return __iscsym(static_cast<unsigned char>(c));
}
