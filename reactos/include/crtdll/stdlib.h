/*
 * stdlib.h
 *
 * Definitions for common types, variables, and functions.
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
 * $Revision: 1.2 $
 * $Author: rex $
 * $Date: 1999/03/19 05:55:09 $
 *
 */
/* Appropriated for Reactos Crtdll by Ariadne */
/* added splitpath */
#ifndef _STDLIB_H_
#define _STDLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This seems like a convenient place to declare these variables, which
 * give programs using WinMain (or main for that matter) access to main-ish
 * argc and argv. environ is a pointer to a table of environment variables.
 * NOTE: Strings in _argv and environ are ANSI strings.
 */
extern int	_argc;
extern char**	_argv;
extern char**	environ;


#define __need_size_t
#define __need_wchar_t
#define __need_NULL
#include <stddef.h>

#ifdef	__GNUC__
#define	_ATTRIB_NORETURN	__attribute__ ((noreturn))
#else	/* Not __GNUC__ */
#define	_ATTRIB_NORETURN
#endif	/* __GNUC__ */

double	atof	(const char* szNumber);
int	atoi	(const char* szNumber);
long	atol	(const char* szNumber);


double	strtod	(const char* szNumber, char** pszAfterNumber);
double	wcstod	(const wchar_t* wsNumber, wchar_t** pwsAfterNumber);
long	strtol	(const char* szNumber, char** pszAfterNumber, int nBase);
long	wcstol	(const wchar_t* wsNumber, wchar_t** pwsAfterNumber, int nBase);

unsigned long	strtoul	(const char* szNumber, char** pszAfterNumber,
			int nBase);
unsigned long	wcstoul (const wchar_t* wsNumber, wchar_t** pwsAfterNumber,
			int nBase);

size_t	wcstombs	(char* mbsDest, const wchar_t* wsConvert, size_t size);
int	wctomb		(char* mbDest, wchar_t wc);

int	mblen		(const char* mbs, size_t sizeString);
size_t	mbstowcs	(wchar_t* wcaDest, const char* mbsConvert,
			 size_t size);
int	mbtowc		(wchar_t* wcDest, const char* mbConvert, size_t size);


/*
 * RAND_MAX is the maximum value that may be returned by rand.
 * The minimum is zero.
 */
#define	RAND_MAX	0x7FFF

int	rand	(void);
void	srand	(unsigned int nSeed);


void*	calloc	(size_t sizeObjCnt, size_t sizeObject);
void*	malloc	(size_t	sizeObject);
void*	realloc	(void* pObject, size_t sizeNew);
void	free	(void* pObject);

/* These values may be used as exit status codes. */
#define	EXIT_SUCCESS	0
#define	EXIT_FAILURE	-1

void	abort	(void) _ATTRIB_NORETURN;
void	exit	(int nStatus) _ATTRIB_NORETURN;
int	atexit	(void (*pfuncExitProcessing)(void));

int	system	(const char* szCommand); // impl in process
char*	getenv	(const char* szVarName); // impl in stdio

typedef	int (*_pfunccmp_t)(const void*, const void*);

void*	bsearch	(const void* pKey, const void* pBase, size_t cntObjects,
		size_t sizeObject, _pfunccmp_t pfuncCmp);
void	qsort	(const void* pBase, size_t cntObjects, size_t sizeObject,
		_pfunccmp_t pfuncCmp);

int	abs	(int n);
long	labs	(long n);

/*
 * div_t and ldiv_t are structures used to return the results of div and
 * ldiv.
 *
 * NOTE: div and ldiv appear not to work correctly unless
 *       -fno-pcc-struct-return is specified. This is included in the
 *       mingw32 specs file.
 */
typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

div_t	div	(int nNumerator, int nDenominator);
ldiv_t	ldiv	(long lNumerator, long lDenominator);
lldiv_t	lldiv	(long long lNumerator, long long lDenominator);


#ifndef	__STRICT_ANSI__

/*
 * NOTE: Officially the three following functions are obsolete. The Win32 API
 *       functions SetErrorMode, Beep and Sleep are their replacements.
 */
void	_beep (unsigned int, unsigned int);
void	_seterrormode (int nMode);
void	_sleep (unsigned long ulTime);

void	_exit	(int nStatus) _ATTRIB_NORETURN;

int	_putenv	(const char* szNameEqValue);
void	_searchenv (const char* szFileName, const char* szVar,
		char* szFullPathBuf);

void _splitpath( const char *path, char *drive, char *dir,
		 char *fname, char *ext );

char*	_itoa (int nValue, char* sz, int nRadix);
char*	_ltoa (long lnValue, char* sz, int nRadix);

char*	_ecvt (double dValue, int nDig, int* pnDec, int* pnSign);
char*	_fcvt (double dValue, int nDig, int* pnDec, int* pnSign);
char*	_gcvt (double dValue, int nDec, char* caBuf);

char*	_fullpath (char* caBuf, const char* szPath, size_t sizeMax);

#ifndef	_NO_OLDNAMES
#define	 beep 		_beep
#define  seterrormode 	_seterrormode 
#define  sleep		_sleep
#define  putenv 	_putenv
#define  searchenv 	_searchenv
#define  splitpath      _splitpath

#define  itoa		_itoa
#define  ltoa 		_ltoa

#define  ecvt		_ecvt
#define  fcvt		_fcvt
#define  gcvt		_gcvt
#endif	/* Not _NO_OLDNAMES */

#endif	/* Not __STRICT_ANSI__ */

/*
 * Undefine the no return attribute used in some function definitions
 */
#undef	_ATTRIB_NORETURN

#ifdef __cplusplus
}
#endif

#endif /* _STDLIB_H_ */
