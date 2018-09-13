/***
*ctype.h - character conversion macros and ctype macros
*
*	Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines macros for character classification/conversion.
*	[ANSI/System V]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_CTYPE
#define _INC_CTYPE

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if	_MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if	_MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else	/* ndef _NTSDK */
/* current definition */
#ifdef	_DLL
#define _CRTIMP __declspec(dllimport)
#else	/* ndef _DLL */
#define _CRTIMP
#endif	/* _DLL */
#endif	/* _NTSDK */
#endif	/* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if	( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#ifndef _MAC
#ifndef _WCTYPE_T_DEFINED
typedef wchar_t wint_t;
typedef wchar_t wctype_t;
#define _WCTYPE_T_DEFINED
#endif

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif
#endif /* ndef _MAC */

/*
 * These declarations allow the user access to the ctype look-up
 * array _ctype defined in ctype.obj by simply including ctype.h
 */
#ifndef _CTYPE_DISABLE_MACROS

#ifdef	_NTSDK

/* Definitions and declarations compatible with the NT SDK */

#ifdef	_DLL

extern unsigned short * _ctype;
#define _pctype     (*_pctype_dll)
extern unsigned short **_pctype_dll;
#define _pwctype    (*_pwctype_dll)
extern unsigned short **_pwctype_dll;

#else	/* _DLL */


extern unsigned short _ctype[];
extern unsigned short *_pctype;
extern wctype_t *_pwctype;

#endif	/* _DLL */

#else	/* ndef _NTSDK */

/* Current declarations */
_CRTIMP extern unsigned short _ctype[];

#if	defined(_DLL) && defined(_M_IX86)

#define _pctype     (*__p__pctype())
_CRTIMP unsigned short ** __cdecl __p__pctype(void);

#define _pwctype    (*__p__pwctype())
_CRTIMP wctype_t ** __cdecl ___p__pwctype(void);

#else	/* !(defined(_DLL) && defined(_M_IX86)) */

_CRTIMP extern unsigned short *_pctype;
#ifndef _MAC
_CRTIMP extern wctype_t *_pwctype;
#endif /* ndef _MAC */

#endif	/* defined(_DLL) && defined(_M_IX86) */

#endif	/* _NTSDK */

#endif	/* _CTYPE_DISABLE_MACROS */

/* set bit masks for the possible character types */

#define _UPPER		0x1	/* upper case letter */
#define _LOWER		0x2	/* lower case letter */
#define _DIGIT		0x4	/* digit[0-9] */
#define _SPACE		0x8	/* tab, carriage return, newline, */
				/* vertical tab or form feed */
#define _PUNCT		0x10	/* punctuation character */
#define _CONTROL	0x20	/* control character */
#define _BLANK		0x40	/* space char */
#define _HEX		0x80	/* hexadecimal digit */

#define _LEADBYTE	0x8000			/* multibyte leadbyte */
#define _ALPHA		(0x0100|_UPPER|_LOWER)	/* alphabetic character */

/* character classification function prototypes */

#ifndef _CTYPE_DEFINED

_CRTIMP int __cdecl _isctype(int, int);
_CRTIMP int __cdecl isalpha(int);
_CRTIMP int __cdecl isupper(int);
_CRTIMP int __cdecl islower(int);
_CRTIMP int __cdecl isdigit(int);
_CRTIMP int __cdecl isxdigit(int);
_CRTIMP int __cdecl isspace(int);
_CRTIMP int __cdecl ispunct(int);
_CRTIMP int __cdecl isalnum(int);
_CRTIMP int __cdecl isprint(int);
_CRTIMP int __cdecl isgraph(int);
_CRTIMP int __cdecl iscntrl(int);
_CRTIMP int __cdecl toupper(int);
_CRTIMP int __cdecl tolower(int);
_CRTIMP int __cdecl _tolower(int);
_CRTIMP int __cdecl _toupper(int);
_CRTIMP int __cdecl __isascii(int);
_CRTIMP int __cdecl __toascii(int);
_CRTIMP int __cdecl __iscsymf(int);
_CRTIMP int __cdecl __iscsym(int);
#define _CTYPE_DEFINED
#endif

#ifndef _MAC
#ifndef _WCTYPE_DEFINED

/* wide function prototypes, also declared in wchar.h  */

/* character classification function prototypes */

_CRTIMP int __cdecl iswalpha(wint_t);
_CRTIMP int __cdecl iswupper(wint_t);
_CRTIMP int __cdecl iswlower(wint_t);
_CRTIMP int __cdecl iswdigit(wint_t);
_CRTIMP int __cdecl iswxdigit(wint_t);
_CRTIMP int __cdecl iswspace(wint_t);
_CRTIMP int __cdecl iswpunct(wint_t);
_CRTIMP int __cdecl iswalnum(wint_t);
_CRTIMP int __cdecl iswprint(wint_t);
_CRTIMP int __cdecl iswgraph(wint_t);
_CRTIMP int __cdecl iswcntrl(wint_t);
_CRTIMP int __cdecl iswascii(wint_t);
_CRTIMP int __cdecl isleadbyte(int);

_CRTIMP wchar_t __cdecl towupper(wchar_t);
_CRTIMP wchar_t __cdecl towlower(wchar_t);

_CRTIMP int __cdecl iswctype(wint_t, wctype_t);

/* --------- The following functions are OBSOLETE --------- */
_CRTIMP int __cdecl is_wctype(wint_t, wctype_t);
/*  --------- The preceding functions are OBSOLETE --------- */

#define _WCTYPE_DEFINED
#endif
#endif /* ndef _MAC */

/* the character classification macro definitions */

#ifndef _CTYPE_DISABLE_MACROS

/*
 * Maximum number of bytes in multi-byte character in the current locale
 * (also defined in stdlib.h).
 */
#ifndef MB_CUR_MAX

#ifdef	_NTSDK

/* definition compatible with NT SDK */
#ifdef	_DLL
#define __mb_cur_max	(*__mb_cur_max_dll)
#define MB_CUR_MAX	(*__mb_cur_max_dll)
extern	unsigned short *__mb_cur_max_dll;
#else	/* ndef _DLL */
#define MB_CUR_MAX __mb_cur_max
extern	unsigned short __mb_cur_max;
#endif	/* _DLL */

#else	/* ndef _NTSDK */

/* current definition */
#if	defined(_DLL) && defined(_M_IX86)
#define MB_CUR_MAX (*__p___mb_cur_max())
_CRTIMP int * __cdecl __p___mb_cur_max(void);
#else	/* !(defined(_DLL) && defined(_M_IX86)) */
#define MB_CUR_MAX __mb_cur_max
_CRTIMP extern int __mb_cur_max;
#endif	/* defined(_DLL) && defined(_M_IX86) */

#endif	/* _NTSDK */

#endif	/* MB_CUR_MAX */

#if defined(_M_MPPC) || defined(_M_M68K)

#ifndef __cplusplus
#define isalpha(_c)	( _pctype[_c] & (_UPPER|_LOWER) )
#define isupper(_c)	( _pctype[_c] & _UPPER )
#define islower(_c)	( _pctype[_c] & _LOWER )
#define isdigit(_c)	( _pctype[_c] & _DIGIT )
#define isxdigit(_c)( _pctype[_c] & _HEX )
#define isspace(_c)	( _pctype[_c] & _SPACE )
#define ispunct(_c)	( _pctype[_c] & _PUNCT )
#define isalnum(_c)	( _pctype[_c] & (_UPPER|_LOWER|_DIGIT) )
#define isprint(_c)	( _pctype[_c] & (_BLANK|_PUNCT|_UPPER|_LOWER|_DIGIT) )
#define isgraph(_c)	( _pctype[_c] & (_PUNCT|_UPPER|_LOWER|_DIGIT) )
#define iscntrl(_c)	( _pctype[_c] & _CONTROL )
#elif 0		/* Pending ANSI C++ integration */
inline int isalpha(int _C)	{return (_pctype[_C] & (_UPPER|_LOWER)); }
inline int isupper(int _C)	{return (_pctype[_C] & _UPPER); }
inline int islower(int _C)	{return (_pctype[_C] & _LOWER); }
inline int isdigit(int _C)	{return (_pctype[_C] & _DIGIT); }
inline int isxdigit(int _C)	{return (_pctype[_C] & _HEX); }
inline int isspace(int _C)	{return (_pctype[_C] & _SPACE); }
inline int ispunct(int _C)	{return (_pctype[_C] & _PUNCT); }
inline int isalnum(int _C)	{return (_pctype[_C] & (_UPPER|_LOWER|_DIGIT)); }
inline int isprint(int _C)
	{return (_pctype[_C] & (_BLANK|_PUNCT|_UPPER|_LOWER|_DIGIT)); }
inline int isgraph(int _C)
	{return (_pctype[_C] & (_PUNCT|_UPPER|_LOWER|_DIGIT)); }
inline int iscntrl(int _C)	{return (_pctype[_C] & _CONTROL); }
#endif	/* __cplusplus */
#else   /* ifdef (MPPC), etc. */
#ifndef __cplusplus
#define isalpha(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_ALPHA) : _pctype[_c] & _ALPHA)
#define isupper(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_UPPER) : _pctype[_c] & _UPPER)
#define islower(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_LOWER) : _pctype[_c] & _LOWER)
#define isdigit(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_DIGIT) : _pctype[_c] & _DIGIT)
#define isxdigit(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_HEX)   : _pctype[_c] & _HEX)
#define isspace(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_SPACE) : _pctype[_c] & _SPACE)
#define ispunct(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_PUNCT) : _pctype[_c] & _PUNCT)
#define isalnum(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_ALPHA|_DIGIT) : _pctype[_c] & (_ALPHA|_DIGIT))
#define isprint(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT) : _pctype[_c] & (_BLANK|_PUNCT|_ALPHA|_DIGIT))
#define isgraph(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_PUNCT|_ALPHA|_DIGIT) : _pctype[_c] & (_PUNCT|_ALPHA|_DIGIT))
#define iscntrl(_c)	(MB_CUR_MAX > 1 ? _isctype(_c,_CONTROL) : _pctype[_c] & _CONTROL)
#elif 0		/* Pending ANSI C++ integration */
inline int isalpha(int _C)
	{return (MB_CUR_MAX > 1 ? _isctype(_C,_ALPHA) : _pctype[_C] & _ALPHA); }
inline int isupper(int _C)
	{return (MB_CUR_MAX > 1 ? _isctype(_C,_UPPER) : _pctype[_C] & _UPPER); }
inline int islower(int _C)
	{return	(MB_CUR_MAX > 1 ? _isctype(_C,_LOWER) : _pctype[_C] & _LOWER); }
inline int isdigit(int _C)
	{return	(MB_CUR_MAX > 1 ? _isctype(_C,_DIGIT) : _pctype[_C] & _DIGIT); }
inline int isxdigit(int _C)
	{return (MB_CUR_MAX > 1 ? _isctype(_C,_HEX)   : _pctype[_C] & _HEX); }
inline int isspace(int _C)
	{return	(MB_CUR_MAX > 1 ? _isctype(_C,_SPACE) : _pctype[_C] & _SPACE); }
inline int ispunct(int _C)
	{return	(MB_CUR_MAX > 1 ? _isctype(_C,_PUNCT) : _pctype[_C] & _PUNCT); }
inline int isalnum(int _C)
	{return	(MB_CUR_MAX > 1 ? _isctype(_C,_ALPHA|_DIGIT)
		: _pctype[_C] & (_ALPHA|_DIGIT)); }
inline int isprint(int _C)
	{return	(MB_CUR_MAX > 1 ? _isctype(_C,_BLANK|_PUNCT|_ALPHA|_DIGIT)
		: _pctype[_C] & (_BLANK|_PUNCT|_ALPHA|_DIGIT)); }
inline int isgraph(int _C)
	{return	(MB_CUR_MAX > 1 ? _isctype(_C,_PUNCT|_ALPHA|_DIGIT)
		: _pctype[_C] & (_PUNCT|_ALPHA|_DIGIT)); }
inline int iscntrl(int _C)
	{return	(MB_CUR_MAX > 1 ? _isctype(_C,_CONTROL)
		: _pctype[_C] & _CONTROL); }
#endif	/* __cplusplus */
#endif	/* _M_MPPC || _M_M68K */

#define _tolower(_c)	( (_c)-'A'+'a' )
#define _toupper(_c)	( (_c)-'a'+'A' )

#define __isascii(_c)	( (unsigned)(_c) < 0x80 )
#define __toascii(_c)	( (_c) & 0x7f )

#ifndef _WCTYPE_INLINE_DEFINED
#ifndef __cplusplus
#define iswalpha(_c)	( iswctype(_c,_ALPHA) )
#define iswupper(_c)	( iswctype(_c,_UPPER) )
#define iswlower(_c)	( iswctype(_c,_LOWER) )
#define iswdigit(_c)	( iswctype(_c,_DIGIT) )
#define iswxdigit(_c)	( iswctype(_c,_HEX) )
#define iswspace(_c)	( iswctype(_c,_SPACE) )
#define iswpunct(_c)	( iswctype(_c,_PUNCT) )
#define iswalnum(_c)	( iswctype(_c,_ALPHA|_DIGIT) )
#define iswprint(_c)	( iswctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT) )
#define iswgraph(_c)	( iswctype(_c,_PUNCT|_ALPHA|_DIGIT) )
#define iswcntrl(_c)	( iswctype(_c,_CONTROL) )
#define iswascii(_c)	( (unsigned)(_c) < 0x80 )

#define isleadbyte(_c)	(_pctype[(unsigned char)(_c)] & _LEADBYTE)
#elif 0		/* __cplusplus */
inline int iswalpha(wint_t _C) {return (iswctype(_C,_ALPHA)); }
inline int iswupper(wint_t _C) {return (iswctype(_C,_UPPER)); }
inline int iswlower(wint_t _C) {return (iswctype(_C,_LOWER)); }
inline int iswdigit(wint_t _C) {return (iswctype(_C,_DIGIT)); }
inline int iswxdigit(wint_t _C) {return (iswctype(_C,_HEX)); }
inline int iswspace(wint_t _C) {return (iswctype(_C,_SPACE)); }
inline int iswpunct(wint_t _C) {return (iswctype(_C,_PUNCT)); }
inline int iswalnum(wint_t _C) {return (iswctype(_C,_ALPHA|_DIGIT)); }
inline int iswprint(wint_t _C)
	{return (iswctype(_C,_BLANK|_PUNCT|_ALPHA|_DIGIT)); }
inline int iswgraph(wint_t _C)
	{return (iswctype(_C,_PUNCT|_ALPHA|_DIGIT)); }
inline int iswcntrl(wint_t _C) {return (iswctype(_C,_CONTROL)); }
inline int iswascii(wint_t _C) {return ((unsigned)(_C) < 0x80); }

inline int isleadbyte(int _C)
	{return (_pctype[(unsigned char)(_C)] & _LEADBYTE); }
#endif	/* __cplusplus */
#define _WCTYPE_INLINE_DEFINED
#endif	/* _WCTYPE_INLINE_DEFINED */

/* MS C version 2.0 extended ctype macros */

#define __iscsymf(_c)	(isalpha(_c) || ((_c) == '_'))
#define __iscsym(_c)	(isalnum(_c) || ((_c) == '_'))

#endif /* _CTYPE_DISABLE_MACROS */


#if	!__STDC__

/* Non-ANSI names for compatibility */

#ifdef	_NTSDK

#define isascii __isascii
#define toascii __toascii
#define iscsymf __iscsymf
#define iscsym	__iscsym

#else	/* ndef _NTSDK */

#ifndef _CTYPE_DEFINED
_CRTIMP int __cdecl isascii(int);
_CRTIMP int __cdecl toascii(int);
_CRTIMP int __cdecl iscsymf(int);
_CRTIMP int __cdecl iscsym(int);
#else
#define isascii __isascii
#define toascii __toascii
#define iscsymf __iscsymf
#define iscsym	__iscsym
#endif

#endif	/* _NTSDK */

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif


#endif	/* _INC_CTYPE */
