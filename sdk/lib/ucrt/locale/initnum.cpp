/***
*initnum.c - contains __acrt_locale_initialize_numeric
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the locale-category initialization function: __acrt_locale_initialize_numeric().
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

static void fix_grouping(
        _Inout_z_   char *  grouping
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

/***
*int __acrt_locale_initialize_numeric() - initialization for LC_NUMERIC locale category.
*
*Purpose:
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

int __cdecl __acrt_locale_initialize_numeric (
        __crt_locale_data* ploci
        )
{
    struct lconv *lc;
    int ret = 0;
    wchar_t* ctrylocalename;
    long *lc_refcount;
    long *lconv_num_refcount = nullptr;
    __crt_locale_pointers locinfo;

    locinfo.locinfo = ploci;
    locinfo.mbcinfo = 0;

    if ( (ploci->locale_name[LC_NUMERIC] != nullptr) ||
         (ploci->locale_name[LC_MONETARY] != nullptr) )
    {
        /*
         * Allocate structure filled with nullptr pointers
         */
        if ( (lc = (struct lconv *)_calloc_crt(1, sizeof(struct lconv)))
             == nullptr )
            return 1;

        /*
         * Copy over all fields (esp., the monetary category)
         */
        *lc = *ploci->lconv;

        /*
         * Allocate a new reference counter for the lconv structure
         */
        if ( (lc_refcount = _malloc_crt_t(long, 1).detach()) == nullptr )
        {
            _free_crt(lc);
            return 1;
        }
        *lc_refcount = 0;

        if ( ploci->locale_name[LC_NUMERIC] != nullptr )
        {
            /*
             * Allocate a new reference counter for the numeric info
             */
            if ( (lconv_num_refcount = _malloc_crt_t(long, 1).detach()) == nullptr )
            {
                _free_crt(lc);
                _free_crt(lc_refcount);
                return 1;
            }
            *lconv_num_refcount = 0;

            /*
             * Numeric data is country--not language--dependent.
             */
            ctrylocalename = ploci->locale_name[LC_NUMERIC];

            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, ctrylocalename, LOCALE_SDECIMAL,
                    (void *)&lc->decimal_point);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, ctrylocalename, LOCALE_STHOUSAND,
                    (void *)&lc->thousands_sep);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, ctrylocalename, LOCALE_SGROUPING,
                    (void *)&lc->grouping);

            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, ctrylocalename, LOCALE_SDECIMAL,
                    (void *)&lc->_W_decimal_point);
            ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, ctrylocalename, LOCALE_STHOUSAND,
                    (void *)&lc->_W_thousands_sep);

            if (ret) {
                    /* Clean up before returning failure */
                    __acrt_locale_free_numeric(lc);
                    _free_crt(lc);
                    _free_crt(lconv_num_refcount);
                    _free_crt(lc_refcount);
                    return -1;
            }

            fix_grouping(lc->grouping);
        }
        else {
            /*
             * C locale for just the numeric category.
             */
            /*
             * nullptr out the reference count pointer
             */
            lconv_num_refcount = nullptr;
            lc->decimal_point = __acrt_lconv_c.decimal_point;
            lc->thousands_sep = __acrt_lconv_c.thousands_sep;
            lc->grouping = __acrt_lconv_c.grouping;
            lc->_W_decimal_point = __acrt_lconv_c._W_decimal_point;
            lc->_W_thousands_sep = __acrt_lconv_c._W_thousands_sep;
        }
        (*lc_refcount) = 1;
        if (lconv_num_refcount)
            (*lconv_num_refcount) = 1;
    }

    else {
        /*
         * C locale for BOTH numeric and monetary categories.
         */
        lconv_num_refcount = nullptr;
        lc_refcount = nullptr;
        lc = &__acrt_lconv_c;           /* point to new one */
    }
    /*
     * If this is part of LC_ALL, then we need to free the old ploci->lconv
     * set up in init_monetary() before this.
     */
    if ( (ploci->lconv_num_refcount != nullptr) &&
         (InterlockedDecrement(ploci->lconv_num_refcount) == 0))
    {
        _ASSERTE(*ploci->lconv_num_refcount > 0);
    }
    if ( (ploci->lconv_intl_refcount != nullptr) &&
         (InterlockedDecrement(ploci->lconv_intl_refcount) == 0))
    {
        _free_crt(ploci->lconv_intl_refcount);
        _free_crt(ploci->lconv);
    }

    ploci->lconv_num_refcount = lconv_num_refcount;
    ploci->lconv_intl_refcount = lc_refcount;

    ploci->lconv = lc;
    return 0;
}

#endif /* _UCRT_ENCLAVE_BUILD */

/*
 *  Free the lconv numeric strings.
 *  Numeric values do not need to be freed.
 */
void __cdecl __acrt_locale_free_numeric(lconv* l)
{
    if (l == nullptr)
        return;

    if ( l->decimal_point != __acrt_lconv_c.decimal_point )
        _free_crt(l->decimal_point);

    if ( l->thousands_sep != __acrt_lconv_c.thousands_sep )
        _free_crt(l->thousands_sep);

    if ( l->grouping != __acrt_lconv_c.grouping )
        _free_crt(l->grouping);

    if ( l->_W_decimal_point != __acrt_lconv_c._W_decimal_point )
        _free_crt(l->_W_decimal_point);

    if ( l->_W_thousands_sep != __acrt_lconv_c._W_thousands_sep )
        _free_crt(l->_W_thousands_sep);
}



} // extern "C"
