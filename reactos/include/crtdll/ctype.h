/* 
 * ctype.h
 *
 * Functions for testing character types and converting characters.
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
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.1.2.1 $
 * $Author: dwelch $
 * $Date: 1999/03/15 16:19:26 $
 *
 */
#ifndef _LINUX_CTYPE_H
#define _LINUX_CTYPE_H

#ifndef _CTYPE_H_
#define _CTYPE_H_

#define	__need_wchar_t
#define	__need_wint_t
#include <stddef.h>


/*
 * The following flags are used to tell iswctype and _isctype what character
 * types you are looking for.
 */
#define	_UPPER		0x0001
#define	_LOWER		0x0002
#define	_DIGIT		0x0004
#define	_SPACE		0x0008
#define	_PUNCT		0x0010
#define	_CONTROL	0x0020
#define	_BLANK		0x0040
#define	_HEX		0x0080
#define	_LEADBYTE	0x8000

#define	_ALPHA		0x0103

/* from DJGPP, see appropriate licence */
#define __dj_ISALNUM 	0x0001
#define __dj_ISALPHA	0x0002
#define __dj_ISCNTRL	0x0004
#define __dj_ISDIGIT	0x0008
#define __dj_ISGRAPH	0x0010
#define __dj_ISLOWER	0x0020
#define __dj_ISPRINT	0x0040
#define __dj_ISPUNCT	0x0080
#define __dj_ISSPACE	0x0100
#define __dj_ISUPPER	0x0200
#define __dj_ISXDIGIT	0x0400

#ifdef __cplusplus
extern "C" {
#endif

int	isalnum(int c);
int	isalpha(int c);
int	iscntrl(int c);
int	isdigit(int c);
int	isgraph(int c);
int	islower(int c);
int	isprint(int c);
int	ispunct(int c);
int	isspace(int c);
int	isupper(int c);
int	isxdigit(int c);

#ifndef	__STRICT_ANSI__
int	_isctype (int c, int ctypeFlags);
#endif

int	tolower(int c);
int	toupper(int c);

/*
 * NOTE: The above are not old name type wrappers, but functions exported
 * explicitly by CRTDLL. However, underscored versions are also exported.
 */
#ifndef	__STRICT_ANSI__
int	_tolower(int c);
int	_toupper(int c);
#endif

#ifndef WEOF
#define	WEOF	(wchar_t)(0xFFFF)
#endif

/*
 * TODO: MB_CUR_MAX should be defined here (if not already defined, since
 *       it should also be defined in stdlib.h). It is supposed to be the
 *       maximum number of bytes in a multi-byte character in the current
 *       locale. Perhaps accessible through the __mb_curr_max_dll entry point,
 *       but I think (again) that that is a variable pointer, which leads to
 *       problems under the current Cygwin compiler distribution.
 */

typedef int	wctype_t;

/* Wide character equivalents */
int	iswalnum(wint_t wc);
int	iswalpha(wint_t wc);
int	iswascii(wint_t wc);
int	iswcntrl(wint_t wc);
int	iswctype(wint_t wc, wctype_t wctypeFlags);
int	is_wctype(wint_t wc, wctype_t wctypeFlags);	/* Obsolete! */
int	iswdigit(wint_t wc);
int	iswgraph(wint_t wc);
int	iswlower(wint_t wc);
int	iswprint(wint_t wc);
int	iswpunct(wint_t wc);
int	iswspace(wint_t wc);
int	iswupper(wint_t wc);
int	iswxdigit(wint_t wc);

wchar_t	towlower(wchar_t c);
wchar_t	towupper(wchar_t c);

int	isleadbyte (int c);

#ifndef	__STRICT_ANSI__
int	__isascii (int c);
int	__toascii (int c);
int	__iscsymf (int c);	/* Valid first character in C symbol */
int	__iscsym (int c);	/* Valid character in C symbol (after first) */

#ifndef	_NO_OLDNAMES
int	isascii (int c);
int	toascii (int c);
int	iscsymf (int c);
int	iscsym (int c);
#endif	/* Not _NO_OLDNAMES */

#endif	/* Not __STRICT_ANSI__ */

#ifdef __cplusplus
}
#endif

#endif
#endif
