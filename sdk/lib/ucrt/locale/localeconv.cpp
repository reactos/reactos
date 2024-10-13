/***
*lconv.c - Contains the localeconv function
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the localeconv() function.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <limits.h>
#include <locale.h>



/* pointer to original static to avoid freeing */
extern "C" { char    __acrt_lconv_static_decimal  []{"."}; }
extern "C" { char    __acrt_lconv_static_null     []{""}; }
extern "C" { wchar_t __acrt_lconv_static_W_decimal[]{L"."}; }
extern "C" { wchar_t __acrt_lconv_static_W_null   []{L""}; }

/* lconv settings for "C" locale */
extern "C" { struct lconv __acrt_lconv_c
{
    __acrt_lconv_static_decimal,   // decimal_point
    __acrt_lconv_static_null,      // thousands_sep
    __acrt_lconv_static_null,      // grouping
    __acrt_lconv_static_null,      // int_curr_symbol
    __acrt_lconv_static_null,      // currency_symbol
    __acrt_lconv_static_null,      // mon_decimal_point
    __acrt_lconv_static_null,      // mon_thousands_sep
    __acrt_lconv_static_null,      // mon_grouping
    __acrt_lconv_static_null,      // positive_sign
    __acrt_lconv_static_null,      // negative_sign
    CHAR_MAX,                      // int_frac_digits
    CHAR_MAX,                      // frac_digits
    CHAR_MAX,                      // p_cs_precedes
    CHAR_MAX,                      // p_sep_by_space
    CHAR_MAX,                      // n_cs_precedes
    CHAR_MAX,                      // n_sep_by_space
    CHAR_MAX,                      // p_sign_posn
    CHAR_MAX,                      // n_sign_posn
    __acrt_lconv_static_W_decimal, // _W_decimal_point
    __acrt_lconv_static_W_null,    // _W_thousands_sep
    __acrt_lconv_static_W_null,    // _W_int_curr_symbol
    __acrt_lconv_static_W_null,    // _W_currency_symbol
    __acrt_lconv_static_W_null,    // _W_mon_decimal_point
    __acrt_lconv_static_W_null,    // _W_mon_thousands_sep
    __acrt_lconv_static_W_null,    // _W_positive_sign
    __acrt_lconv_static_W_null,    // _W_negative_sign
}; }


/* pointer to current lconv structure */

extern "C" { struct lconv* __acrt_lconv{&__acrt_lconv_c}; }

/***
*struct lconv *localeconv(void) - Return the numeric formatting convention
*
*Purpose:
*       The localeconv() routine returns the numeric formatting conventions
*       for the current locale setting.  [ANSI]
*
*Entry:
*       void
*
*Exit:
*       struct lconv * = pointer to struct indicating current numeric
*                        formatting conventions.
*
*Exceptions:
*
*******************************************************************************/

extern "C" lconv* __cdecl localeconv()
{
    // Note that we don't need _LocaleUpdate in this function.  The main reason
    // being, that this is a leaf function in locale usage terms.
    __acrt_ptd* const ptd = __acrt_getptd();
    __crt_locale_data* locale_info = ptd->_locale_info;

    __acrt_update_locale_info(ptd, &locale_info);

    return locale_info->lconv;
}
