/***
*ctype.h - character conversion macros and ctype macros
*
*       Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines macros for character classification/conversion.
*       [ANSI/System V]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_CTYPE
#define _INC_CTYPE

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <cruntime.h>

/* Define _CRTAPI1 (for compatibility with the NT SDK) */
#ifndef _CRTAPI1
#if _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#define _CRTAPI1
#endif  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#endif  /* _CRTAPI1 */


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#define _CRTAPI2
#endif  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#endif  /* _CRTAPI2 */


/* Define _CRTIMP */

#ifndef _CRTIMP
/* current definition */
#ifdef CRTDLL
#define _CRTIMP __declspec(dllexport)
#else  /* CRTDLL */
#ifdef _DLL
#define _CRTIMP __declspec(dllimport)
#else  /* _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _CRTIMP */


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif  /* _WCHAR_T_DEFINED */

#ifndef _MAC
#ifndef _WCTYPE_T_DEFINED
typedef wchar_t wint_t;
typedef wchar_t wctype_t;
#define _WCTYPE_T_DEFINED
#endif  /* _WCTYPE_T_DEFINED */

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif  /* WEOF */
#endif  /* _MAC */

/*
 * These declarations allow the user access to the ctype look-up
 * array _ctype defined in ctype.obj by simply including ctype.h
 */
#ifndef _CTYPE_DISABLE_MACROS


/* Current declarations */
_CRTIMP extern unsigned short _ctype[];

#if defined (_DLL) && defined (_M_IX86)

#define _pctype     (*__p__pctype())
_CRTIMP unsigned short ** __cdecl __p__pctype(void);

#define _pwctype    (*__p__pwctype())
_CRTIMP wctype_t ** __cdecl ___p__pwctype(void);

#else  /* defined (_DLL) && defined (_M_IX86) */

_CRTIMP extern unsigned short *_pctype;
#ifndef _MAC
_CRTIMP extern wctype_t *_pwctype;
#endif  /* _MAC */

#endif  /* defined (_DLL) && defined (_M_IX86) */


#endif  /* _CTYPE_DISABLE_MACROS */



/* set bit masks for the possible character types */

#define _UPPER          0x1     /* upper case letter */
#define _LOWER          0x2     /* lower case letter */
#define _DIGIT          0x4     /* digit[0-9] */
#define _SPACE          0x8     /* tab, carriage return, newline, */
                                /* vertical tab or form feed */
#define _PUNCT          0x10    /* punctuation character */
#define _CONTROL        0x20    /* control character */
#define _BLANK          0x40    /* space char */
#define _HEX            0x80    /* hexadecimal digit */

#define _LEADBYTE       0x8000                  /* multibyte leadbyte */
#define _ALPHA          (0x0100|_UPPER|_LOWER)  /* alphabetic character */

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
#endif  /* _CTYPE_DEFINED */

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

#define _WCTYPE_DEFINED
#endif  /* _WCTYPE_DEFINED */
#endif  /* _MAC */

/* the character classification macro definitions */

#ifndef _CTYPE_DISABLE_MACROS

/*
 * Maximum number of bytes in multi-byte character in the current locale
 * (also defined in stdlib.h).
 */
#ifndef MB_CUR_MAX


/* current definition */
#if defined (_DLL) && defined (_M_IX86)
#define MB_CUR_MAX (*__p___mb_cur_max())
_CRTIMP int * __cdecl __p___mb_cur_max(void);
#else  /* defined (_DLL) && defined (_M_IX86) */
#define MB_CUR_MAX __mb_cur_max
_CRTIMP extern int __mb_cur_max;
#endif  /* defined (_DLL) && defined (_M_IX86) */


#endif  /* MB_CUR_MAX */

#ifndef __cplusplus
#define isalpha(_c)     ( _pctype[_c] & (_UPPER|_LOWER) )
#define isupper(_c)     ( _pctype[_c] & _UPPER )
#define islower(_c)     ( _pctype[_c] & _LOWER )
#define isdigit(_c)     ( _pctype[_c] & _DIGIT )
#define isxdigit(_c)( _pctype[_c] & _HEX )
#define isspace(_c)     ( _pctype[_c] & _SPACE )
#define ispunct(_c)     ( _pctype[_c] & _PUNCT )
#define isalnum(_c)     ( _pctype[_c] & (_UPPER|_LOWER|_DIGIT) )
#define isprint(_c)     ( _pctype[_c] & (_BLANK|_PUNCT|_UPPER|_LOWER|_DIGIT) )
#define isgraph(_c)     ( _pctype[_c] & (_PUNCT|_UPPER|_LOWER|_DIGIT) )
#define iscntrl(_c)     ( _pctype[_c] & _CONTROL )
#endif  /* __cplusplus */

#define _tolower(_c)    ( (_c)-'A'+'a' )
#define _toupper(_c)    ( (_c)-'a'+'A' )

#define __isascii(_c)   ( (unsigned)(_c) < 0x80 )
#define __toascii(_c)   ( (_c) & 0x7f )

#ifndef _WCTYPE_INLINE_DEFINED
#ifndef __cplusplus
#define iswalpha(_c)    ( iswctype(_c,_ALPHA) )
#define iswupper(_c)    ( iswctype(_c,_UPPER) )
#define iswlower(_c)    ( iswctype(_c,_LOWER) )
#define iswdigit(_c)    ( iswctype(_c,_DIGIT) )
#define iswxdigit(_c)   ( iswctype(_c,_HEX) )
#define iswspace(_c)    ( iswctype(_c,_SPACE) )
#define iswpunct(_c)    ( iswctype(_c,_PUNCT) )
#define iswalnum(_c)    ( iswctype(_c,_ALPHA|_DIGIT) )
#define iswprint(_c)    ( iswctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT) )
#define iswgraph(_c)    ( iswctype(_c,_PUNCT|_ALPHA|_DIGIT) )
#define iswcntrl(_c)    ( iswctype(_c,_CONTROL) )
#define iswascii(_c)    ( (unsigned)(_c) < 0x80 )

#define isleadbyte(_c)  (_pctype[(unsigned char)(_c)] & _LEADBYTE)
#endif  /* __cplusplus */
#define _WCTYPE_INLINE_DEFINED
#endif  /* _WCTYPE_INLINE_DEFINED */

#endif  /* _CTYPE_DISABLE_MACROS */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_CTYPE */
