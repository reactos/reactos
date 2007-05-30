/* 
 * locale.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Functions and types for localization (ie. changing the appearance of
 * output based on the standards of a certain country).
 *
 */

#ifndef	_LOCALE_H_
#define	_LOCALE_H_

/* All the headers include this file. */
#include <_mingw.h>

/*
 * NOTE: I have tried to test this, but I am limited by my knowledge of
 *       locale issues. The structure does not bomb if you look at the
 *       values, and 'decimal_point' even seems to be correct. But the
 *       rest of the values are, by default, not particularly useful
 *       (read meaningless and not related to the international settings
 *       of the system).
 */

#define	LC_ALL		0
#define LC_COLLATE	1
#define LC_CTYPE	2
#define	LC_MONETARY	3
#define	LC_NUMERIC	4
#define	LC_TIME		5
#define	LC_MIN		LC_ALL
#define	LC_MAX		LC_TIME

#ifndef RC_INVOKED

/* According to C89 std, NULL is defined in locale.h too.  */
#define __need_NULL
#include <stddef.h>

/*
 * The structure returned by 'localeconv'.
 */
struct lconv
{
	char*	decimal_point;
	char*	thousands_sep;
	char*	grouping;
	char*	int_curr_symbol;
	char*	currency_symbol;
	char*	mon_decimal_point;
	char*	mon_thousands_sep;
	char*	mon_grouping;
	char*	positive_sign;
	char*	negative_sign;
	char	int_frac_digits;
	char	frac_digits;
	char	p_cs_precedes;
	char	p_sep_by_space;
	char	n_cs_precedes;
	char	n_sep_by_space;
	char	p_sign_posn;
	char	n_sign_posn;
};

#ifdef	__cplusplus
extern "C" {
#endif

_CRTIMP  char* __cdecl setlocale (int, const char*);
_CRTIMP struct lconv* __cdecl localeconv (void);

#ifndef _WLOCALE_DEFINED  /* also declared in wchar.h */
# define __need_wchar_t
# include <stddef.h>
  _CRTIMP wchar_t* __cdecl _wsetlocale(int, const wchar_t*);
# define _WLOCALE_DEFINED
#endif /* ndef _WLOCALE_DEFINED */

#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _LOCALE_H_ */

