/*
 * string.h
 *
 * Definitions for memory and string functions.
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
 * $Author: chorns $
 * $Date: 2001/09/23 22:14:03 $
 *
 */
/* Appropriated for Reactos Crtdll by Ariadne */
/* changed prototype for _strerror */
/* moved prototype for swab from string.h to stdlib.h */


#ifndef _STRING_H_
#define	_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Define size_t, wchar_t and NULL
 */
#define __need_size_t
#define __need_wchar_t
#define	__need_NULL
#include <msvcrt/stddef.h>

char * ___strtok; // removed extern specifier 02-06-98, BD

/*
 * Prototypes of the ANSI Standard C library string functions.
 */
void*	memchr (const void* p, int cSearchFor, size_t sizeSearch);
int 	memcmp (const void* p1, const void* p2, size_t sizeSearch);
void* 	memcpy (void* pCopyTo, const void* pSource, size_t sizeSource);
void*	memmove (void* pMoveTo, const void* pSource, size_t sizeSource);
void*	memset (void* p, int cFill, size_t sizeRepeatCount);
char*	strcat (char* szAddTo, const char* szAdd);
char*	strchr (const char* szSearch, int cFor);
int	strcmp (const char* sz1, const char* sz2);
int	strcoll (const char* sz1, const char* sz2); /* Compare using locale */
char*	strcpy (char* szCopyTo, const char* szSource);
size_t	strcspn (const char* szGetPrefix, const char* szNotIncluding);
char*	strerror (int nError); /* NOTE: NOT an old name wrapper. */
char *  _strerror(const char *s);
size_t	strlen (const char* sz);
size_t	strnlen (const char* sz, size_t count); // not exported
char*	strncat (char* szAddTo, const char* szAdd, size_t sizeMaxAdd);
int	strncmp (const char* sz1, const char* sz2, size_t sizeMaxCompare);
char*	strncpy (char* szCopyTo, const char* szSource, size_t sizeMaxCopy);
char*	strpbrk (const char* szSearch, const char* szAnyOf);
char*	strrchr (const char* szSearch, int cFor);
size_t	strspn (const char* szGetPrefix, const char *szIncluding);
char*	strstr (const char* szSearch, const char *szFor);
char*	strtok (char* szTokenize, const char* szDelimiters);
size_t	strxfrm (char* szTransformed, const char *szSource,
	         size_t sizeTransform);

#ifndef __STRICT_ANSI__
/*
 * Extra non-ANSI functions provided by the CRTDLL library
 */
void*	_memccpy (void* pCopyTo, const void* pSource, int cTerminator,
	          size_t sizeMaxCopy);
int	_memicmp (const void* p1, const void* p2, size_t sizeSearch);
char*	_strdup (const char *szDuplicate);
int	_strcmpi (const char* sz1, const char* sz2);
int	_stricmp (const char* sz1, const char* sz2);
int	_stricoll (const char* sz1, const char* sz2);
char*	_strlwr (char* szToConvert);
int	_strnicmp (const char* sz1, const char* sz2,
	           size_t sizeMaxCompare);
char*	_strnset (char* szToFill, int cFill, size_t sizeMaxFill);
char*	_strrev (char* szToReverse);
char*	_strset (char* szToFill, int cFill);
char*	_strupr (char* szToConvert);


#endif	/* Not __STRICT_ANSI__ */


/*
 * Unicode versions of the standard calls.
 */
wchar_t* wcscat (wchar_t* wsAddTo, const wchar_t* wsAdd);
wchar_t* wcschr (const wchar_t* wsSearch, wchar_t wcFor);
int	wcscmp (const wchar_t* ws1, const wchar_t* ws2);
int	wcscoll (const wchar_t* ws1, const wchar_t* ws2);
wchar_t* wcscpy (wchar_t* wsCopyTo, const wchar_t* wsSource);
size_t	wcscspn (const wchar_t* wsGetPrefix, const wchar_t* wsNotIncluding);
/* Note: No wcserror in CRTDLL. */
size_t	wcslen (const wchar_t* ws);
wchar_t* wcsncat (wchar_t* wsAddTo, const wchar_t* wsAdd, size_t sizeMaxAdd);
int	wcsncmp(const wchar_t* ws1, const wchar_t* ws2, size_t sizeMaxCompare);
wchar_t* wcsncpy(wchar_t* wsCopyTo, const wchar_t* wsSource,
                 size_t sizeMaxCopy);
wchar_t* wcspbrk(const wchar_t* wsSearch, const wchar_t* wsAnyOf);
wchar_t* wcsrchr(const wchar_t* wsSearch, wchar_t wcFor);
size_t	wcsspn(const wchar_t* wsGetPrefix, const wchar_t* wsIncluding);
wchar_t* wcsstr(const wchar_t* wsSearch, const wchar_t* wsFor);
wchar_t* wcstok(wchar_t* wsTokenize, const wchar_t* wsDelimiters);
size_t	wcsxfrm(wchar_t* wsTransformed, const wchar_t *wsSource,
	        size_t sizeTransform);


#ifndef	__STRICT_ANSI__
/*
 * Unicode versions of non-ANSI functions provided by CRTDLL.
 */

/* NOTE: _wcscmpi not provided by CRTDLL, this define is for portability */
#define		_wcscmpi	_wcsicmp

wchar_t* _wcsdup (const wchar_t* wsToDuplicate);
int	_wcsicmp (const wchar_t* ws1, const wchar_t* ws2);
int	_wcsicoll (const wchar_t* ws1, const wchar_t* ws2);
int	_wcsncoll (const wchar_t *s1, const wchar_t *s2, size_t c);
int	_wcsnicoll (const wchar_t *s1, const wchar_t *s2, size_t c);
wchar_t* _wcslwr (wchar_t* wsToConvert);
int	_wcsnicmp (const wchar_t* ws1, const wchar_t* ws2,
	           size_t sizeMaxCompare);
wchar_t* _wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill);
wchar_t* _wcsrev (wchar_t* wsToReverse);
wchar_t* _wcsset (wchar_t* wsToFill, wchar_t wcToFill);
wchar_t* _wcsupr (wchar_t* wsToConvert);

#endif	/* Not __STRICT_ANSI__ */


#ifndef	__STRICT_ANSI__
#ifndef	_NO_OLDNAMES

/*
 * Non-underscored versions of non-ANSI functions. They live in liboldnames.a
 * and provide a little extra portability. Also a few extra UNIX-isms like
 * strcasecmp.
 */

void*	memccpy (void* pCopyTo, const void* pSource, int cTerminator,
	         size_t sizeMaxCopy);
int	memicmp (const void* p1, const void* p2, size_t sizeSearch);
#define	strdup(szDuplicate)	_strdup(szDuplicate)
int	strcmpi (const char* sz1, const char* sz2);
int	stricmp (const char* sz1, const char* sz2);
int	strcasecmp (const char* sz1, const char* sz2);
int	stricoll (const char* sz1, const char* sz2);
char*	strlwr (char* szToConvert);
int	strnicmp (const char* sz1, const char* sz2, size_t sizeMaxCompare);
int	strncasecmp (const char* sz1, const char* sz2, size_t sizeMaxCompare);
char*	strnset (char* szToFill, int cFill, size_t sizeMaxFill);
char*	strrev (char* szToReverse);
char*	strset (char* szToFill, int cFill);
char*	strupr (char* szToConvert);


/* NOTE: There is no _wcscmpi, but this is for compatibility. */
int	wcscmpi	(const wchar_t* ws1, const wchar_t* ws2);
wchar_t* wcsdup (const wchar_t* wsToDuplicate);
int	wcsicmp (const wchar_t* ws1, const wchar_t* ws2);
int	wcsicoll (const wchar_t* ws1, const wchar_t* ws2);
wchar_t* wcslwr (wchar_t* wsToConvert);
int	wcsnicmp (const wchar_t* ws1, const wchar_t* ws2,
	          size_t sizeMaxCompare);
wchar_t* wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill);
wchar_t* wcsrev (wchar_t* wsToReverse);
wchar_t* wcsset (wchar_t* wsToFill, wchar_t wcToFill);
wchar_t* wcsupr (wchar_t* wsToConvert);

#endif	/* Not _NO_OLDNAMES */
#endif	/* Not strict ANSI */

#endif

#ifdef __cplusplus
extern "C" }
#endif
