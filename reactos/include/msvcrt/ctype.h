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
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.5 $
 * $Author: robd $
 * $Date: 2002/11/24 18:06:00 $
 *
 */

#ifndef _CTYPE_H_
#define _CTYPE_H_

#define __need_wchar_t
#define __need_wint_t
#include <msvcrt/stddef.h>


/*
 * The following flags are used to tell iswctype and _isctype what character
 * types you are looking for.
 */
#define _UPPER      0x0001
#define _LOWER      0x0002
#define _DIGIT      0x0004
#define _SPACE      0x0008 /* HT  LF  VT  FF  CR  SP */
#define _PUNCT      0x0010
#define _CONTROL    0x0020
#define _BLANK      0x0040 /* this is SP only, not SP and HT as in C99  */
#define _HEX        0x0080
#define _LEADBYTE   0x8000

#define _ALPHA      0x0103


#ifdef __cplusplus
extern "C" {
#endif

int isalnum(int);
int isalpha(int);
int iscntrl(int);
int isdigit(int);
int isgraph(int);
int islower(int);
int isprint(int);
int ispunct(int);
int isspace(int);
int isupper(int);
int isxdigit(int);

#ifndef __STRICT_ANSI__
int _isctype (unsigned int, int);
#endif

/* These are the ANSI versions, with correct checking of argument */
int tolower(int);
int toupper(int);

/*
 * NOTE: The above are not old name type wrappers, but functions exported
 * explicitly by MSVCRT/CRTDLL. However, underscored versions are also
 * exported.
 */
#ifndef __STRICT_ANSI__
/*
 *  These are the cheap non-std versions: The return values are undefined
 *  if the argument is not ASCII char or is not of appropriate case
 */ 
int _tolower(int);
int _toupper(int);
#endif

/*
 * TODO: MB_CUR_MAX should be defined here (if not already defined, since
 *       it should also be defined in stdlib.h). It is supposed to be the
 *       maximum number of bytes in a multi-byte character in the current
 *       locale. Perhaps accessible through the __mb_curr_max_dll entry point,
 *       but I think (again) that that is a variable pointer, which leads to
 *       problems under the current Cygwin compiler distribution.
 */

typedef int wctype_t;


/* Wide character equivalents */
#ifndef WEOF
#define WEOF    (wchar_t)(0xFFFF)
#endif

int iswalnum(wint_t);
int iswalpha(wint_t);
int iswascii(wint_t);
int iswcntrl(wint_t);
int iswctype(wint_t, wctype_t);
int is_wctype(wint_t, wctype_t);    /* Obsolete! */
int iswdigit(wint_t);
int iswgraph(wint_t);
int iswlower(wint_t);
int iswprint(wint_t);
int iswpunct(wint_t);
int iswspace(wint_t);
int iswupper(wint_t);
int iswxdigit(wint_t);

wchar_t towlower(wchar_t);
wchar_t towupper(wchar_t);

int isleadbyte(int);

#ifndef __STRICT_ANSI__
int __isascii(int);
int __toascii(int);
int __iscsymf(int);    /* Valid first character in C symbol */
int __iscsym(int); /* Valid character in C symbol (after first) */

#ifndef _NO_OLDNAMES
#define isascii(c)  __isascii(c)
#define toascii(c)  _toascii(c)
#define iscsymf(c)  __iscsymf(c)
#define iscsym(c)   __iscsym(c)
#endif  /* Not _NO_OLDNAMES */

#endif  /* Not __STRICT_ANSI__ */

#ifdef __cplusplus
}
#endif

#endif  /* Not _CTYPE_H_ */
