/***
*locale.h - definitions/declarations for localization routines
*
*   Copyright (c) 1988-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the structures, values, macros, and functions
*   used by the localization routines.
*   [ANSI]
*
****/

#ifndef _INC_LOCALE

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

/* define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else 
#define NULL    ((void *)0)
#endif 
#endif 


/* Locale categories */

#define LC_ALL      0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5

#define LC_MIN      LC_ALL
#define LC_MAX      LC_TIME


/* Locale convention structure */

#ifndef _LCONV_DEFINED
struct lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
    };
#define _LCONV_DEFINED
#endif 

/* function prototypes */

char * __cdecl setlocale(int, const char *);
struct lconv * __cdecl localeconv(void);

#ifdef __cplusplus
}
#endif 

#define _INC_LOCALE
#endif 
