/*
 * ctype.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Functions for testing character types and converting characters.
 *
 */

#ifndef _CTYPE_H_
#define _CTYPE_H_

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
#define	_SPACE		0x0008 /* HT  LF  VT  FF  CR  SP */
#define	_PUNCT		0x0010
#define	_CONTROL	0x0020
/* _BLANK is set for SP and non-ASCII horizontal space chars (eg,
   "no-break space", 0xA0, in CP1250) but not for HT.  */
#define	_BLANK		0x0040
#define	_HEX		0x0080
#define	_LEADBYTE	0x8000

#define	_ALPHA		0x0103

#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif

_CRTIMP int __cdecl isalnum(int);
_CRTIMP int __cdecl isalpha(int);
_CRTIMP int __cdecl iscntrl(int);
_CRTIMP int __cdecl isdigit(int);
_CRTIMP int __cdecl isgraph(int);
_CRTIMP int __cdecl islower(int);
_CRTIMP int __cdecl isprint(int);
_CRTIMP int __cdecl ispunct(int);
_CRTIMP int __cdecl isspace(int);
_CRTIMP int __cdecl isupper(int);
_CRTIMP int __cdecl isxdigit(int);

#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
     || !defined __STRICT_ANSI__
int __cdecl isblank (int);
#endif

#ifndef __STRICT_ANSI__
_CRTIMP int __cdecl _isctype (int, int);
#endif

/* These are the ANSI versions, with correct checking of argument */
_CRTIMP int __cdecl tolower(int);
_CRTIMP int __cdecl toupper(int);

/*
 * NOTE: The above are not old name type wrappers, but functions exported
 * explicitly by MSVCRT/CRTDLL. However, underscored versions are also
 * exported.
 */
#ifndef	__STRICT_ANSI__
/*
 *  These are the cheap non-std versions: The return values are undefined
 *  if the argument is not ASCII char or is not of appropriate case
 */
_CRTIMP int __cdecl _tolower(int);
_CRTIMP int __cdecl _toupper(int);
#endif

/* Also defined in stdlib.h */
#ifndef MB_CUR_MAX
#ifdef __DECLSPEC_SUPPORTED
# ifdef __MSVCRT__
#  define MB_CUR_MAX __mb_cur_max
   __MINGW_IMPORT int __mb_cur_max;
# else	/* not __MSVCRT */
#  define MB_CUR_MAX __mb_cur_max_dll
   __MINGW_IMPORT int __mb_cur_max_dll;
# endif	/* not __MSVCRT */

#else		/* ! __DECLSPEC_SUPPORTED */
# ifdef __MSVCRT__
   extern int* _imp____mbcur_max;
#  define MB_CUR_MAX (*_imp____mb_cur_max)
# else		/* not __MSVCRT */
   extern int*  _imp____mbcur_max_dll;
#  define MB_CUR_MAX (*_imp____mb_cur_max_dll)
# endif 	/* not __MSVCRT */
#endif  	/*  __DECLSPEC_SUPPORTED */
#endif  /* MB_CUR_MAX */


#ifdef __DECLSPEC_SUPPORTED
__MINGW_IMPORT unsigned short _ctype[];
# ifdef __MSVCRT__
  __MINGW_IMPORT unsigned short* _pctype;
# else /* CRTDLL */
  __MINGW_IMPORT unsigned short* _pctype_dll;
# define  _pctype _pctype_dll
# endif

#else		/*  __DECLSPEC_SUPPORTED */
extern unsigned short** _imp___ctype;
#define _ctype (*_imp___ctype)
# ifdef __MSVCRT__
  extern unsigned short** _imp___pctype;
# define _pctype (*_imp___pctype)
# else /* CRTDLL */
  extern unsigned short** _imp___pctype_dll;
# define _pctype (*_imp___pctype_dll)
# endif /* CRTDLL */
#endif		/*  __DECLSPEC_SUPPORTED */

/*
 * Use inlines here rather than macros, because macros will upset
 * C++ usage (eg, ::isalnum), and so usually get undefined
 *
 * According to standard for SB chars, these function are defined only
 * for input values representable by unsigned char or EOF.
 * Thus, there is no range test.
 * This reproduces behaviour of MSVCRT.dll lib implemention for SB chars.
 *
 * If no MB char support is needed, these can be simplified even
 * more by command line define -DMB_CUR_MAX=1.  The compiler will then
 * optimise away the constant condition.
 */

#if ! (defined (__NO_INLINE__)  || defined (__NO_CTYPE_INLINES) \
	|| defined (__STRICT_ANSI__))

/* use  simple lookup if SB locale, else  _isctype()  */
#define __ISCTYPE(c, mask)  (MB_CUR_MAX == 1 ? (_pctype[c] & mask) : _isctype(c, mask))
__CRT_INLINE int __cdecl isalnum(int c) {return __ISCTYPE(c, (_ALPHA|_DIGIT));}
__CRT_INLINE int __cdecl isalpha(int c) {return __ISCTYPE(c, _ALPHA);}
__CRT_INLINE int __cdecl iscntrl(int c) {return __ISCTYPE(c, _CONTROL);}
__CRT_INLINE int __cdecl isdigit(int c) {return __ISCTYPE(c, _DIGIT);}
__CRT_INLINE int __cdecl isgraph(int c) {return __ISCTYPE(c, (_PUNCT|_ALPHA|_DIGIT));}
__CRT_INLINE int __cdecl islower(int c) {return __ISCTYPE(c, _LOWER);}
__CRT_INLINE int __cdecl isprint(int c) {return __ISCTYPE(c, (_BLANK|_PUNCT|_ALPHA|_DIGIT));}
__CRT_INLINE int __cdecl ispunct(int c) {return __ISCTYPE(c, _PUNCT);}
__CRT_INLINE int __cdecl isspace(int c) {return __ISCTYPE(c, _SPACE);}
__CRT_INLINE int __cdecl isupper(int c) {return __ISCTYPE(c, _UPPER);}
__CRT_INLINE int __cdecl isxdigit(int c) {return __ISCTYPE(c, _HEX);}

#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
     || !defined __STRICT_ANSI__
__CRT_INLINE int __cdecl isblank (int c)
  {return (__ISCTYPE(c, _BLANK) || c == '\t');}
#endif

/* these reproduce behaviour of lib underscored versions  */
__CRT_INLINE int __cdecl _tolower(int c) {return ( c -'A'+'a');}
__CRT_INLINE int __cdecl _toupper(int c) {return ( c -'a'+'A');}

/* TODO? Is it worth inlining ANSI tolower, toupper? Probably only
   if we only want C-locale. */

#endif /* _NO_CTYPE_INLINES */

/* Wide character equivalents */

#ifndef WEOF
#define	WEOF	(wchar_t)(0xFFFF)
#endif

#ifndef _WCTYPE_T_DEFINED
typedef wchar_t wctype_t;
#define _WCTYPE_T_DEFINED
#endif

_CRTIMP int __cdecl iswalnum(wint_t);
_CRTIMP int __cdecl iswalpha(wint_t);
_CRTIMP int __cdecl iswascii(wint_t);
_CRTIMP int __cdecl iswcntrl(wint_t);
_CRTIMP int __cdecl iswctype(wint_t, wctype_t);
_CRTIMP int __cdecl is_wctype(wint_t, wctype_t);	/* Obsolete! */
_CRTIMP int __cdecl iswdigit(wint_t);
_CRTIMP int __cdecl iswgraph(wint_t);
_CRTIMP int __cdecl iswlower(wint_t);
_CRTIMP int __cdecl iswprint(wint_t);
_CRTIMP int __cdecl iswpunct(wint_t);
_CRTIMP int __cdecl iswspace(wint_t);
_CRTIMP int __cdecl iswupper(wint_t);
_CRTIMP int __cdecl iswxdigit(wint_t);

#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
     || !defined __STRICT_ANSI__ || defined __cplusplus
int __cdecl iswblank (wint_t);
#endif

/* Older MS docs uses wchar_t for arg and return type, while newer
   online MS docs say arg is wint_t and return is int.
   ISO C uses wint_t for both.  */
_CRTIMP wint_t __cdecl towlower (wint_t);
_CRTIMP wint_t __cdecl towupper (wint_t);

_CRTIMP int __cdecl isleadbyte (int);

/* Also in wctype.h */
#if ! (defined (__NO_INLINE__) || defined(__NO_CTYPE_INLINES) \
       || defined(__WCTYPE_INLINES_DEFINED))
#define __WCTYPE_INLINES_DEFINED
__CRT_INLINE int __cdecl iswalnum(wint_t wc) {return (iswctype(wc,_ALPHA|_DIGIT));}
__CRT_INLINE int __cdecl iswalpha(wint_t wc) {return (iswctype(wc,_ALPHA));}
__CRT_INLINE int __cdecl iswascii(wint_t wc) {return ((wc & ~0x7F) ==0);}
__CRT_INLINE int __cdecl iswcntrl(wint_t wc) {return (iswctype(wc,_CONTROL));}
__CRT_INLINE int __cdecl iswdigit(wint_t wc) {return (iswctype(wc,_DIGIT));}
__CRT_INLINE int __cdecl iswgraph(wint_t wc) {return (iswctype(wc,_PUNCT|_ALPHA|_DIGIT));}
__CRT_INLINE int __cdecl iswlower(wint_t wc) {return (iswctype(wc,_LOWER));}
__CRT_INLINE int __cdecl iswprint(wint_t wc) {return (iswctype(wc,_BLANK|_PUNCT|_ALPHA|_DIGIT));}
__CRT_INLINE int __cdecl iswpunct(wint_t wc) {return (iswctype(wc,_PUNCT));}
__CRT_INLINE int __cdecl iswspace(wint_t wc) {return (iswctype(wc,_SPACE));}
__CRT_INLINE int __cdecl iswupper(wint_t wc) {return (iswctype(wc,_UPPER));}
__CRT_INLINE int __cdecl iswxdigit(wint_t wc) {return (iswctype(wc,_HEX));}
__CRT_INLINE int __cdecl isleadbyte(int c) {return (_pctype[(unsigned char)(c)] & _LEADBYTE);}
#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
     || !defined __STRICT_ANSI__ || defined __cplusplus
__CRT_INLINE int __cdecl iswblank (wint_t wc)
  {return (iswctype(wc,_BLANK) || wc == L'\t');}
#endif

#endif /* !(defined(__NO_CTYPE_INLINES) || defined(__WCTYPE_INLINES_DEFINED)) */

#ifndef	__STRICT_ANSI__
int __cdecl __isascii (int);
int __cdecl __toascii (int);
int __cdecl __iscsymf (int);		/* Valid first character in C symbol */
int __cdecl __iscsym (int);		/* Valid character in C symbol (after first) */

#if !(defined (__NO_INLINE__) || defined (__NO_CTYPE_INLINES))
__CRT_INLINE int __cdecl __isascii(int c) {return ((c & ~0x7F) == 0);}
__CRT_INLINE int __cdecl __toascii(int c) {return (c & 0x7F);}
__CRT_INLINE int __cdecl __iscsymf(int c) {return (isalpha(c) || (c == '_'));}
__CRT_INLINE int __cdecl __iscsym(int c)  {return  (isalnum(c) || (c == '_'));}
#endif /* __NO_CTYPE_INLINES */

#ifndef	_NO_OLDNAMES
/* Not _CRTIMP */
int __cdecl isascii (int);
int __cdecl toascii (int);
int __cdecl iscsymf (int);
int __cdecl iscsym (int);
#endif	/* Not _NO_OLDNAMES */

#endif	/* Not __STRICT_ANSI__ */

#ifdef __cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _CTYPE_H_ */

