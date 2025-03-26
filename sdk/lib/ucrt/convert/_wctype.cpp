//
// _wctype.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the function equivalents of the wide character classification macros
// in <ctype.h>.  These function definitions make use of the macros.  The
// functions return zero if the character does not meet the requirements, and
// nonzero if the character does meet the requirements.
//
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>

#define __inline


extern "C" __inline int (__cdecl _isleadbyte_l)(int const c, _locale_t const locale)
{
    _LocaleUpdate locale_update(locale);
    return __acrt_locale_get_ctype_array_value(locale_update.GetLocaleT()->locinfo->_public._locale_pctype, c, _LEADBYTE);
}

extern "C" __inline int (__cdecl isleadbyte)(int const c)
{
    return (_isleadbyte_l)(c, nullptr);
}

extern "C" __inline int (__cdecl _iswalpha_l)(wint_t const c, _locale_t)
{
    return iswalpha(c);
}

extern "C" __inline int (__cdecl iswalpha)(wint_t const c)
{
    return iswalpha(c);
}

extern "C" __inline int (__cdecl _iswupper_l)(wint_t const c, _locale_t)
{
    return iswupper(c);
}

extern "C" __inline int (__cdecl iswupper)(wint_t const c)
{
    return iswupper(c);
}

extern "C" __inline int (__cdecl _iswlower_l)(wint_t const c, _locale_t)
{
    return iswlower(c);
}

extern "C" __inline int (__cdecl iswlower)(wint_t const c)
{
    return iswlower(c);
}

extern "C" __inline int (__cdecl _iswdigit_l)(wint_t const c, _locale_t)
{
    return iswdigit(c);
}

extern "C" __inline int (__cdecl iswdigit)(wint_t const c)
{
    return iswdigit(c);
}

extern "C" __inline int (__cdecl _iswxdigit_l)(wint_t const c, _locale_t)
{
    return iswxdigit(c);
}

extern "C" __inline int (__cdecl iswxdigit)(wint_t const c)
{
    return iswxdigit(c);
}

extern "C" __inline int (__cdecl _iswspace_l)(wint_t const c, _locale_t)
{
    return iswspace(c);
}

extern "C" __inline int (__cdecl iswspace)(wint_t const c)
{
    return iswspace(c);
}

extern "C" __inline int (__cdecl _iswpunct_l)(wint_t const c, _locale_t)
{
    return iswpunct(c);
}

extern "C" __inline int (__cdecl iswpunct)(wint_t const c)
{
    return iswpunct(c);
}

extern "C" __inline int (__cdecl _iswblank_l)(wint_t const c, _locale_t)
{
    return iswblank(c);
}

extern "C" __inline int (__cdecl iswblank)(wint_t const c)
{
    return iswblank(c);
}

extern "C" __inline int (__cdecl _iswalnum_l)(wint_t const c, _locale_t)
{
    return iswalnum(c);
}

extern "C" __inline int (__cdecl iswalnum)(wint_t const c)
{
    return iswalnum(c);
}

extern "C" __inline int (__cdecl _iswprint_l)(wint_t const c, _locale_t)
{
    return iswprint(c);
}

extern "C" __inline int (__cdecl iswprint)(wint_t const c)
{
    return iswprint(c);
}

extern "C" __inline int (__cdecl _iswgraph_l)(wint_t const c, _locale_t)
{
    return iswgraph(c);
}

extern "C" __inline int (__cdecl iswgraph)(wint_t const c)
{
    return iswgraph(c);
}

extern "C" __inline int (__cdecl _iswcntrl_l)(wint_t const c, _locale_t)
{
    return iswcntrl(c);
}

extern "C" __inline int (__cdecl iswcntrl)(wint_t const c)
{
    return iswcntrl(c);
}

extern "C" __inline int (__cdecl iswascii)(wint_t const c)
{
    return iswascii(c);
}

extern "C" __inline int (__cdecl _iswcsym_l)(wint_t const c, _locale_t)
{
    return __iswcsym(c);
}

extern "C" __inline int (__cdecl __iswcsym)(wint_t const c)
{
    return __iswcsym(c);
}

extern "C" __inline int (__cdecl _iswcsymf_l)(wint_t const c, _locale_t)
{
    return __iswcsymf(c);
}

extern "C" __inline int (__cdecl __iswcsymf)(wint_t const c)
{
    return __iswcsymf(c);
}
