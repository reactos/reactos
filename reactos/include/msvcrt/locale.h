/* 
 * locale.h
 *
 * Functions and types for localization (ie. changing the appearance of
 * output based on the standards of a certain country).
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.4 $
 * $Author: robd $
 * $Date: 2002/11/24 18:06:00 $
 *
 */

#ifndef _LOCALE_H_
#define _LOCALE_H_


/*
 * NOTE: I have tried to test this, but I am limited by my knowledge of
 *       locale issues. The structure does not bomb if you look at the
 *       values, and 'decimal_point' even seems to be correct. But the
 *       rest of the values are, by default, not particularly useful
 *       (read meaningless and not related to the international settings
 *       of the system).
 */

#define LC_ALL      0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5
#define LC_MIN      LC_ALL
#define LC_MAX      LC_TIME

/*
 * The structure returned by 'localeconv'.
 */
struct lconv
{
    char*   decimal_point;
    char*   thousands_sep;
    char*   grouping;
    char*   int_curr_symbol;
    char*   currency_symbol;
    char*   mon_decimal_point;
    char*   mon_thousands_sep;
    char*   mon_grouping;
    char*   positive_sign;
    char*   negative_sign;
    char    int_frac_digits;
    char    frac_digits;
    char    p_cs_precedes;
    char    p_sep_by_space;
    char    n_cs_precedes;
    char    n_sep_by_space;
    char    p_sign_posn;
    char    n_sign_posn;
};

#ifdef  __cplusplus
extern "C" {
#endif

char* setlocale(int, const char*);
struct lconv* localeconv(void);

#ifdef  __cplusplus
}
#endif

#endif  /* Not _LOCALE_H_ */

