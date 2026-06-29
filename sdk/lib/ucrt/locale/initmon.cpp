/***
*initmon.c - contains __acrt_locale_initialize_monetary
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the locale-category initialization function: __acrt_locale_initialize_monetary().
*
*       Each initialization function sets up locale-specific information
*       for their category, for use by functions which are affected by
*       their locale category.
*
*       *** For internal use by setlocale() only ***
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <locale.h>

extern "C" {


    // Enclaves have no ability to create new locales.
#ifndef _UCRT_ENCLAVE_BUILD

static void fix_grouping(_Inout_z_ char *);

/*
 *  Note that __acrt_lconv_c is used when the monetary category is in the C locale
 *  but the numeric category may not necessarily be in the C locale.
 */


/***
*int __acrt_locale_initialize_monetary() - initialization for LC_MONETARY locale category.
*
*Purpose:
*       In non-C locales, read the localized monetary strings into
*       __acrt_lconv_intl, and also copy the numeric strings from __acrt_lconv into
*       __acrt_lconv_intl.  Set __acrt_lconv to point to __acrt_lconv_intl.  The old
*       __acrt_lconv_intl is not freed until the new one is fully established.
*
*       In the C locale, the monetary fields in lconv are filled with
*       contain C locale values.  Any allocated __acrt_lconv_intl fields are freed.
*
*       At startup, __acrt_lconv points to a static lconv structure containing
*       C locale strings.  This structure is never used again if
*       __acrt_locale_initialize_monetary is called.
*
*Entry:
*       None.
*
*Exit:
*       0 success
*       1 fail
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __acrt_locale_initialize_monetary (
        __crt_locale_data* ploci
        )
{
    struct lconv *lc;
    int ret;
    wchar_t* ctrylocalename;
    long *lc_refcount;
    long *lconv_mon_refcount = nullptr;
    __crt_locale_pointers locinfo;

    locinfo.locinfo = ploci;
    locinfo.mbcinfo = 0;

    if ( (ploci->locale_name[LC_MONETARY] != nullptr) ||
         (ploci->locale_name[LC_NUMERIC] != nullptr) )
    {
        /*
         * Allocate structure filled with nullptr pointers
         */
        if ((lc = _calloc_crt_t(lconv, 1).detach()) == nullptr)
            return 1;

        /*
         * Allocate a new reference counter for the lconv structure
         */
        if ( (lc_refcount = _calloc_crt_t(long, 1).detach()) == nullptr )
        {
            _free_crt(lc);
            return 1;
        }

        if ( ploci->locale_name[LC_MONETARY] != nullptr )
        {
            /*
             * Allocate a new reference counter for the numeric info
             */
            if ( (lconv_mon_refcount = _calloc_crt_t(long, 1).detach()) == nullptr )
            {
                _free_crt(lc);
                _free_crt(lc_refcount);
                return 1;
            }

            /*
             * Currency is country--not language--dependent. NT
             * work-around.
             */
            ctrylocalename = ploci->locale_name[LC_MONETARY];

            ret = 0;

            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, ctrylocalename,
                    LOCALE_SINTLSYMBOL, (void *)&lc->int_curr_symbol );
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, ctrylocalename,
                    LOCALE_SCURRENCY, (void *)&lc->currency_symbol );
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, ctrylocalename,
                    LOCALE_SMONDECIMALSEP, (void *)&lc->mon_decimal_point );
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, ctrylocalename,
                    LOCALE_SMONTHOUSANDSEP, (void *)&lc->mon_thousands_sep );
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, ctrylocalename,
                    LOCALE_SMONGROUPING, (void *)&lc->mon_grouping );

            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, ctrylocalename,
                    LOCALE_SPOSITIVESIGN, (void *)&lc->positive_sign);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, ctrylocalename,
                    LOCALE_SNEGATIVESIGN, (void *)&lc->negative_sign);

            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_INT_TYPE, ctrylocalename,
                    LOCALE_IINTLCURRDIGITS, (void *)&lc->int_frac_digits);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_INT_TYPE, ctrylocalename,
                    LOCALE_ICURRDIGITS, (void *)&lc->frac_digits);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_INT_TYPE, ctrylocalename,
                    LOCALE_IPOSSYMPRECEDES, (void *)&lc->p_cs_precedes);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_INT_TYPE, ctrylocalename,
                    LOCALE_IPOSSEPBYSPACE, (void *)&lc->p_sep_by_space);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_INT_TYPE, ctrylocalename,
                    LOCALE_INEGSYMPRECEDES, (void *)&lc->n_cs_precedes);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_INT_TYPE, ctrylocalename,
                    LOCALE_INEGSEPBYSPACE, (void *)&lc->n_sep_by_space);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_INT_TYPE, ctrylocalename,
                    LOCALE_IPOSSIGNPOSN, (void *)&lc->p_sign_posn);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_INT_TYPE, ctrylocalename,
                    LOCALE_INEGSIGNPOSN, (void *)&lc->n_sign_posn);

            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, ctrylocalename,
                    LOCALE_SINTLSYMBOL, (void *)&lc->_W_int_curr_symbol );
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, ctrylocalename,
                    LOCALE_SCURRENCY, (void *)&lc->_W_currency_symbol );
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, ctrylocalename,
                    LOCALE_SMONDECIMALSEP, (void *)&lc->_W_mon_decimal_point );
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, ctrylocalename,
                    LOCALE_SMONTHOUSANDSEP, (void *)&lc->_W_mon_thousands_sep );
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, ctrylocalename,
                    LOCALE_SPOSITIVESIGN, (void *)&lc->_W_positive_sign);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, ctrylocalename,
                    LOCALE_SNEGATIVESIGN, (void *)&lc->_W_negative_sign);

            if ( ret != 0 ) {
                __acrt_locale_free_monetary(lc);
                _free_crt(lc);
                _free_crt(lc_refcount);
                _free_crt(lconv_mon_refcount);
                return 1;
            }

            fix_grouping(lc->mon_grouping);
        }
        else {
            /*
             * C locale for monetary category (the numeric category fields,
             * which are NOT of the C locale, get fixed up below). Note
             * that __acrt_lconv_c is copied, rather than directly assigning
             * the fields of lc because of the uncertainty of the values of
             * the int_frac_digits,..., n_sign_posn fields (SCHAR_MAX or
             * UCHAR_MAX, depending on whether or a compliand was built
             * with -J.
             */
            *lc = __acrt_lconv_c;
        }

        /*
         * Copy the numeric locale fields from the old struct
         */
        lc->decimal_point = ploci->lconv->decimal_point;
        lc->thousands_sep = ploci->lconv->thousands_sep;
        lc->grouping = ploci->lconv->grouping;
        lc->_W_decimal_point = ploci->lconv->_W_decimal_point;
        lc->_W_thousands_sep = ploci->lconv->_W_thousands_sep;

        *lc_refcount = 1;
        if (lconv_mon_refcount)
            *lconv_mon_refcount = 1;
    }
    else {
        /*
         * C locale for BOTH monetary and numeric categories.
         */
        lconv_mon_refcount = nullptr;
        lc_refcount = nullptr;
        lc = &__acrt_lconv_c;           /* point to new one */

    }

    if ( (ploci->lconv_mon_refcount != nullptr) &&
         (InterlockedDecrement(ploci->lconv_mon_refcount) == 0))
    {
        _ASSERTE(*ploci->lconv_mon_refcount > 0);
    }
    if ( (ploci->lconv_intl_refcount != nullptr) &&
         (InterlockedDecrement(ploci->lconv_intl_refcount) == 0))
    {
        _free_crt(ploci->lconv);
        _free_crt(ploci->lconv_intl_refcount);
    }
    ploci->lconv_mon_refcount = lconv_mon_refcount;
    ploci->lconv_intl_refcount = lc_refcount;
    ploci->lconv = lc;                       /* point to new one */

    return 0;
}

static void fix_grouping(
        char *grouping
        )
{
    /*
     * ANSI specifies that the fields should contain "\3" [\3\0] to indicate
     * thousands groupings (100,000,000.00 for example).
     * NT uses "3;0"; ASCII 3 instead of value 3 and the ';' is extra.
     * So here we convert the NT version to the ANSI version.
     */

    while (*grouping)
    {
        /* convert '3' to '\3' */
        if (*grouping >= '0' && *grouping <= '9')
        {
            *grouping = *grouping - '0';
            grouping++;
        }

        /* remove ';' */
        else if (*grouping == ';')
        {
            char *tmp = grouping;

            do
                *tmp = *(tmp+1);
            while (*++tmp);
        }

        /* unknown (illegal) character, ignore */
        else
            grouping++;
    }
}

#endif /* _UCRT_ENCLAVE_BUILD */

/*
 *  Free the lconv monetary strings.
 *  Numeric values do not need to be freed.
 */
void __cdecl __acrt_locale_free_monetary(lconv* l)
{
    if (l == nullptr)
        return;

    if ( l->int_curr_symbol != __acrt_lconv_c.int_curr_symbol )
        _free_crt(l->int_curr_symbol);

    if ( l->currency_symbol != __acrt_lconv_c.currency_symbol )
        _free_crt(l->currency_symbol);

    if ( l->mon_decimal_point != __acrt_lconv_c.mon_decimal_point )
        _free_crt(l->mon_decimal_point);

    if ( l->mon_thousands_sep != __acrt_lconv_c.mon_thousands_sep )
        _free_crt(l->mon_thousands_sep);

    if ( l->mon_grouping != __acrt_lconv_c.mon_grouping )
        _free_crt(l->mon_grouping);

    if ( l->positive_sign != __acrt_lconv_c.positive_sign )
        _free_crt(l->positive_sign);

    if ( l->negative_sign != __acrt_lconv_c.negative_sign )
        _free_crt(l->negative_sign);

    if ( l->_W_int_curr_symbol != __acrt_lconv_c._W_int_curr_symbol )
        _free_crt(l->_W_int_curr_symbol);

    if ( l->_W_currency_symbol != __acrt_lconv_c._W_currency_symbol )
        _free_crt(l->_W_currency_symbol);

    if ( l->_W_mon_decimal_point != __acrt_lconv_c._W_mon_decimal_point )
        _free_crt(l->_W_mon_decimal_point);

    if ( l->_W_mon_thousands_sep != __acrt_lconv_c._W_mon_thousands_sep )
        _free_crt(l->_W_mon_thousands_sep);

    if ( l->_W_positive_sign != __acrt_lconv_c._W_positive_sign )
        _free_crt(l->_W_positive_sign);

    if ( l->_W_negative_sign != __acrt_lconv_c._W_negative_sign )
        _free_crt(l->_W_negative_sign);
}



} // extern "C"
