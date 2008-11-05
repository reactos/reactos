/*
 * wctype.h
 *
 * Functions for testing wide character types and converting characters.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Mumit Khan <khan@xraylith.wisc.edu>
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
 */

#ifndef _WCTYPE_H_
#define _WCTYPE_H_

/* All the headers include this file. */
#include <_mingw.h>

#define	__need_wchar_t
#define	__need_wint_t
#ifndef RC_INVOKED
#include <stddef.h>
#endif	/* Not RC_INVOKED */

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

#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEOF
#define	WEOF	(wchar_t)(0xFFFF)
#endif

#ifndef _WCTYPE_T_DEFINED
typedef wchar_t wctype_t;
#define _WCTYPE_T_DEFINED
#endif

/* Wide character equivalents - also in ctype.h */
_CRTIMP int __cdecl __MINGW_NOTHROW	iswctype(wint_t, wctype_t);
_CRTIMP int __cdecl __MINGW_NOTHROW	is_wctype(wint_t, wctype_t);	/* Obsolete! */

#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
     || !defined __STRICT_ANSI__ || defined __cplusplus
int __cdecl __MINGW_NOTHROW iswblank (wint_t);
#endif

/* Older MS docs uses wchar_t for arg and return type, while newer
   online MS docs say arg is wint_t and return is int.
   ISO C uses wint_t for both.  */
_CRTIMP wint_t __cdecl __MINGW_NOTHROW	towlower (wint_t);
_CRTIMP wint_t __cdecl __MINGW_NOTHROW	towupper (wint_t);

_CRTIMP int __cdecl __MINGW_NOTHROW	isleadbyte (int);

/* Also in ctype.h */

#ifndef _CTYPE_H_
#ifdef __DECLSPEC_SUPPORTED
# if __MSVCRT_VERSION__ <= 0x0700
  __MINGW_IMPORT unsigned short _ctype[];
# endif
# ifdef __MSVCRT__
  __MINGW_IMPORT const unsigned short* _pctype;
# else	/* CRTDLL */
  __MINGW_IMPORT const unsigned short* _pctype_dll;
# define  _pctype _pctype_dll
# endif

#else		/* ! __DECLSPEC_SUPPORTED */
# if __MSVCRT_VERSION__ <= 0x0700
  extern unsigned short** _imp___ctype;
# define _ctype (*_imp___ctype)
# endif
# ifdef __MSVCRT__
  extern unsigned short** _imp___pctype;
# define _pctype (*_imp___pctype)
# else	/* CRTDLL */
  extern unsigned short** _imp___pctype_dll;
# define _pctype (*_imp___pctype_dll)
# endif	/* CRTDLL */
#endif		/*  __DECLSPEC_SUPPORTED */
#endif

#if !(defined (__NO_INLINE__) || defined(__NO_CTYPE_INLINES) \
      || defined(__WCTYPE_INLINES_DEFINED))
#define __WCTYPE_INLINES_DEFINED
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswalnum(wint_t wc) {return (iswctype(wc,_ALPHA|_DIGIT));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswalpha(wint_t wc) {return (iswctype(wc,_ALPHA));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswascii(wint_t wc) {return ((wc & ~0x7F) ==0);}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswcntrl(wint_t wc) {return (iswctype(wc,_CONTROL));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswdigit(wint_t wc) {return (iswctype(wc,_DIGIT));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswgraph(wint_t wc) {return (iswctype(wc,_PUNCT|_ALPHA|_DIGIT));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswlower(wint_t wc) {return (iswctype(wc,_LOWER));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswprint(wint_t wc) {return (iswctype(wc,_BLANK|_PUNCT|_ALPHA|_DIGIT));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswpunct(wint_t wc) {return (iswctype(wc,_PUNCT));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswspace(wint_t wc) {return (iswctype(wc,_SPACE));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswupper(wint_t wc) {return (iswctype(wc,_UPPER));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswxdigit(wint_t wc) {return (iswctype(wc,_HEX));}
__CRT_INLINE int __cdecl __MINGW_NOTHROW isleadbyte(int c) {return (_pctype[(unsigned char)(c)] & _LEADBYTE);}

#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
     || !defined __STRICT_ANSI__ || defined __cplusplus
__CRT_INLINE int __cdecl __MINGW_NOTHROW iswblank (wint_t wc)
  {return (iswctype(wc, _BLANK) || wc == L'\t');}
#endif

#endif /* !(defined(__NO_CTYPE_INLINES) || defined(__WCTYPE_INLINES_DEFINED)) */

#ifndef __WCTYPE_INLINES_DEFINED
_CRTIMP int __cdecl __MINGW_NOTHROW iswalnum(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswalpha(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswascii(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswcntrl(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswdigit(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswgraph(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswlower(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswprint(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswpunct(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswspace(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswupper(wint_t);
_CRTIMP int __cdecl __MINGW_NOTHROW iswxdigit(wint_t);
#endif

typedef wchar_t wctrans_t;

/* These are resolved by libmingwex.a.  Note, that they are also exported
   by the MS C++ runtime lib (msvcp60.dll).  The msvcp60.dll implementations
   of wctrans and towctrans are not C99 compliant in that wctrans("tolower")
   returns 0, while std specifies that a non-zero value should be returned
   for a valid string descriptor.  If you want the MS behaviour (and you have
   msvcp60.dll in your path) add -lmsvcp60 to your command line.  */

wint_t __cdecl __MINGW_NOTHROW		towctrans(wint_t, wctrans_t);
wctrans_t __cdecl __MINGW_NOTHROW	wctrans(const char*);
wctype_t __cdecl __MINGW_NOTHROW	wctype(const char*);

#ifdef __cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _WCTYPE_H_ */


