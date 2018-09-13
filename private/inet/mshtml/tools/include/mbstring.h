/***
* mbstring.h - MBCS string manipulation macros and functions
*
*	Copyright (c) 1990-1995, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file contains macros and function declarations for the MBCS
*	string manipulation functions.
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_MBSTRING
#define _INC_MBSTRING

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef	_MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif	/* _MSC_VER */

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


#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif


#ifndef _NLSCMP_DEFINED
#define _NLSCMPERROR    2147483647	/* currently == INT_MAX */
#define _NLSCMP_DEFINED
#endif


#ifndef _VA_LIST_DEFINED
#ifdef	_M_ALPHA
typedef struct {
	char *a0;	/* pointer to first homed integer argument */
	int offset;	/* byte offset of next parameter */
} va_list;
#else
typedef char *	va_list;
#endif
#define _VA_LIST_DEFINED
#endif

#ifndef _FILE_DEFINED
struct _iobuf {
	char *_ptr;
	int   _cnt;
	char *_base;
	int   _flag;
	int   _file;
	int   _charbuf;
	int   _bufsiz;
	char *_tmpfname;
	};
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif

/*
 * MBCS - Multi-Byte Character Set
 */

#ifndef _MBSTRING_DEFINED

/* function prototypes */

_CRTIMP unsigned int __cdecl _mbbtombc(unsigned int);
_CRTIMP int __cdecl _mbbtype(unsigned char, int);
_CRTIMP unsigned int __cdecl _mbctombb(unsigned int);
_CRTIMP int __cdecl _mbsbtype(const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbscat(unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbschr(const unsigned char *, unsigned int);
_CRTIMP int __cdecl _mbscmp(const unsigned char *, const unsigned char *);
_CRTIMP int __cdecl _mbscoll(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbscpy(unsigned char *, const unsigned char *);
_CRTIMP size_t __cdecl _mbscspn(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsdec(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsdup(const unsigned char *);
_CRTIMP int __cdecl _mbsicmp(const unsigned char *, const unsigned char *);
_CRTIMP int __cdecl _mbsicoll(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsinc(const unsigned char *);
_CRTIMP size_t __cdecl _mbslen(const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbslwr(unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsnbcat(unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbcmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbcoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP size_t __cdecl _mbsnbcnt(const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsnbcpy(unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbicmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbicoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsnbset(unsigned char *, unsigned int, size_t);
_CRTIMP unsigned char * __cdecl _mbsncat(unsigned char *, const unsigned char *, size_t);
_CRTIMP size_t __cdecl _mbsnccnt(const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsncmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsncoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsncpy(unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned int __cdecl _mbsnextc (const unsigned char *);
_CRTIMP int __cdecl _mbsnicmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnicoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsninc(const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsnset(unsigned char *, unsigned int, size_t);
_CRTIMP unsigned char * __cdecl _mbspbrk(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsrchr(const unsigned char *, unsigned int);
_CRTIMP unsigned char * __cdecl _mbsrev(unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsset(unsigned char *, unsigned int);
_CRTIMP size_t __cdecl _mbsspn(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsspnp(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsstr(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbstok(unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsupr(unsigned char *);

_CRTIMP size_t __cdecl _mbclen(const unsigned char *);
_CRTIMP void __cdecl _mbccpy(unsigned char *, const unsigned char *);
#define _mbccmp(_cpc1, _cpc2) _mbsncmp((_cpc1),(_cpc2),1)

/* character routines */

_CRTIMP int __cdecl _ismbcalnum(unsigned int);
_CRTIMP int __cdecl _ismbcalpha(unsigned int);
_CRTIMP int __cdecl _ismbcdigit(unsigned int);
_CRTIMP int __cdecl _ismbcgraph(unsigned int);
_CRTIMP int __cdecl _ismbclegal(unsigned int);
_CRTIMP int __cdecl _ismbclower(unsigned int);
_CRTIMP int __cdecl _ismbcprint(unsigned int);
_CRTIMP int __cdecl _ismbcpunct(unsigned int);
_CRTIMP int __cdecl _ismbcspace(unsigned int);
_CRTIMP int __cdecl _ismbcupper(unsigned int);

_CRTIMP unsigned int __cdecl _mbctolower(unsigned int);
_CRTIMP unsigned int __cdecl _mbctoupper(unsigned int);

#define _MBSTRING_DEFINED
#endif

#ifndef _MBLEADTRAIL_DEFINED
_CRTIMP int __cdecl _ismbblead( unsigned int );
_CRTIMP int __cdecl _ismbbtrail( unsigned int );
_CRTIMP int __cdecl _ismbslead( const unsigned char *, const unsigned char *);
_CRTIMP int __cdecl _ismbstrail( const unsigned char *, const unsigned char *);
#define _MBLEADTRAIL_DEFINED
#endif

/*  Kanji specific prototypes.	*/

_CRTIMP int __cdecl _ismbchira(unsigned int);
_CRTIMP int __cdecl _ismbckata(unsigned int);
_CRTIMP int __cdecl _ismbcsymbol(unsigned int);
_CRTIMP int __cdecl _ismbcl0(unsigned int);
_CRTIMP int __cdecl _ismbcl1(unsigned int);
_CRTIMP int __cdecl _ismbcl2(unsigned int);
_CRTIMP unsigned int __cdecl _mbcjistojms(unsigned int);
_CRTIMP unsigned int __cdecl _mbcjmstojis(unsigned int);
_CRTIMP unsigned int __cdecl _mbctohira(unsigned int);
_CRTIMP unsigned int __cdecl _mbctokata(unsigned int);

#ifdef __cplusplus
}
#endif

#ifdef	_MSC_VER
#pragma pack(pop)
#endif	/* _MSC_VER */

#endif	/* _INC_MBSTRING */
