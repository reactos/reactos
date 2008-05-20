/*
 * string.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Definitions for memory and string functions.
 *
 */

#ifndef _STRING_H_
#define	_STRING_H_

/* All the headers include this file. */
#include <_mingw.h>

/*
 * Define size_t, wchar_t and NULL
 */
#define __need_size_t
#define __need_wchar_t
#define	__need_NULL
#ifndef RC_INVOKED
#include <stddef.h>
#endif	/* Not RC_INVOKED */

#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Prototypes of the ANSI Standard C library string functions.
 */
_CRTIMP void* __cdecl __MINGW_NOTHROW	memchr (const void*, int, size_t) __MINGW_ATTRIB_PURE;
_CRTIMP int __cdecl __MINGW_NOTHROW 	memcmp (const void*, const void*, size_t) __MINGW_ATTRIB_PURE;
_CRTIMP void* __cdecl __MINGW_NOTHROW 	memcpy (void*, const void*, size_t);
_CRTIMP void* __cdecl __MINGW_NOTHROW	memmove (void*, const void*, size_t);
_CRTIMP void* __cdecl __MINGW_NOTHROW	memset (void*, int, size_t);
_CRTIMP char* __cdecl __MINGW_NOTHROW	strcat (char*, const char*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	strchr (const char*, int)  __MINGW_ATTRIB_PURE;
_CRTIMP int __cdecl __MINGW_NOTHROW	strcmp (const char*, const char*)  __MINGW_ATTRIB_PURE;
_CRTIMP int __cdecl __MINGW_NOTHROW	strcoll (const char*, const char*);	/* Compare using locale */
_CRTIMP char* __cdecl __MINGW_NOTHROW	strcpy (char*, const char*);
_CRTIMP size_t __cdecl __MINGW_NOTHROW	strcspn (const char*, const char*)  __MINGW_ATTRIB_PURE;
_CRTIMP char* __cdecl __MINGW_NOTHROW	strerror (int); /* NOTE: NOT an old name wrapper. */

_CRTIMP size_t __cdecl __MINGW_NOTHROW	strlen (const char*)  __MINGW_ATTRIB_PURE;
_CRTIMP char* __cdecl __MINGW_NOTHROW	strncat (char*, const char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW	strncmp (const char*, const char*, size_t)  __MINGW_ATTRIB_PURE;
_CRTIMP char* __cdecl __MINGW_NOTHROW	strncpy (char*, const char*, size_t);
_CRTIMP char* __cdecl __MINGW_NOTHROW	strpbrk (const char*, const char*)  __MINGW_ATTRIB_PURE;
_CRTIMP char* __cdecl __MINGW_NOTHROW	strrchr (const char*, int)  __MINGW_ATTRIB_PURE;
_CRTIMP size_t __cdecl __MINGW_NOTHROW	strspn (const char*, const char*)  __MINGW_ATTRIB_PURE;
_CRTIMP char* __cdecl __MINGW_NOTHROW	strstr (const char*, const char*)  __MINGW_ATTRIB_PURE;
_CRTIMP char* __cdecl __MINGW_NOTHROW	strtok (char*, const char*);
_CRTIMP size_t __cdecl __MINGW_NOTHROW	strxfrm (char*, const char*, size_t);

#ifndef __STRICT_ANSI__
/*
 * Extra non-ANSI functions provided by the CRTDLL library
 */
_CRTIMP char* __cdecl __MINGW_NOTHROW	_strerror (const char *);
_CRTIMP void* __cdecl __MINGW_NOTHROW	_memccpy (void*, const void*, int, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW 	_memicmp (const void*, const void*, size_t);
_CRTIMP char* __cdecl __MINGW_NOTHROW 	_strdup (const char*) __MINGW_ATTRIB_MALLOC;
_CRTIMP int __cdecl __MINGW_NOTHROW	_strcmpi (const char*, const char*);
_CRTIMP int __cdecl __MINGW_NOTHROW	_stricmp (const char*, const char*);
_CRTIMP int __cdecl __MINGW_NOTHROW	_stricoll (const char*, const char*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_strlwr (char*);
_CRTIMP int __cdecl __MINGW_NOTHROW	_strnicmp (const char*, const char*, size_t);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_strnset (char*, int, size_t);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_strrev (char*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_strset (char*, int);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_strupr (char*);
_CRTIMP void __cdecl __MINGW_NOTHROW	_swab (const char*, char*, size_t);

#ifdef __MSVCRT__
_CRTIMP int __cdecl __MINGW_NOTHROW  _strncoll(const char*, const char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW  _strnicoll(const char*, const char*, size_t);
#endif

#ifndef	_NO_OLDNAMES
/*
 * Non-underscored versions of non-ANSI functions. They live in liboldnames.a
 * and provide a little extra portability. Also a few extra UNIX-isms like
 * strcasecmp.
 */
_CRTIMP void* __cdecl __MINGW_NOTHROW	memccpy (void*, const void*, int, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW	memicmp (const void*, const void*, size_t);
_CRTIMP char* __cdecl __MINGW_NOTHROW	strdup (const char*) __MINGW_ATTRIB_MALLOC;
_CRTIMP int __cdecl __MINGW_NOTHROW	strcmpi (const char*, const char*);
_CRTIMP int __cdecl __MINGW_NOTHROW	stricmp (const char*, const char*);
__CRT_INLINE int __cdecl __MINGW_NOTHROW strcasecmp (const char*, const char *);
__CRT_INLINE int __cdecl __MINGW_NOTHROW
strcasecmp (const char * __sz1, const char * __sz2)
  {return _stricmp (__sz1, __sz2);}
_CRTIMP int __cdecl __MINGW_NOTHROW	stricoll (const char*, const char*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	strlwr (char*);
_CRTIMP int __cdecl __MINGW_NOTHROW	strnicmp (const char*, const char*, size_t);
__CRT_INLINE int  __cdecl __MINGW_NOTHROW strncasecmp (const char *, const char *, size_t);
__CRT_INLINE int __cdecl __MINGW_NOTHROW
strncasecmp (const char * __sz1, const char * __sz2, size_t __sizeMaxCompare)
  {return _strnicmp (__sz1, __sz2, __sizeMaxCompare);}
_CRTIMP char* __cdecl __MINGW_NOTHROW	strnset (char*, int, size_t);
_CRTIMP char* __cdecl __MINGW_NOTHROW	strrev (char*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	strset (char*, int);
_CRTIMP char* __cdecl __MINGW_NOTHROW	strupr (char*);
#ifndef _UWIN
_CRTIMP void __cdecl __MINGW_NOTHROW	swab (const char*, char*, size_t);
#endif /* _UWIN */
#endif /* _NO_OLDNAMES */

#endif	/* Not __STRICT_ANSI__ */

#ifndef _WSTRING_DEFINED
/*
 * Unicode versions of the standard calls.
 * Also in wchar.h, where they belong according to ISO standard.
 */
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcscat (wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcschr (const wchar_t*, wchar_t);
_CRTIMP int __cdecl __MINGW_NOTHROW	wcscmp (const wchar_t*, const wchar_t*);
_CRTIMP int __cdecl __MINGW_NOTHROW	wcscoll (const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcscpy (wchar_t*, const wchar_t*);
_CRTIMP size_t __cdecl __MINGW_NOTHROW	wcscspn (const wchar_t*, const wchar_t*);
/* Note:  _wcserror requires __MSVCRT_VERSION__ >= 0x0700.  */
_CRTIMP size_t __cdecl __MINGW_NOTHROW	wcslen (const wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcsncat (wchar_t*, const wchar_t*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW	wcsncmp(const wchar_t*, const wchar_t*, size_t);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcsncpy(wchar_t*, const wchar_t*, size_t);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcspbrk(const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcsrchr(const wchar_t*, wchar_t);
_CRTIMP size_t __cdecl __MINGW_NOTHROW	wcsspn(const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcsstr(const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcstok(wchar_t*, const wchar_t*);
_CRTIMP size_t __cdecl __MINGW_NOTHROW	wcsxfrm(wchar_t*, const wchar_t*, size_t);

#ifndef	__STRICT_ANSI__
/*
 * Unicode versions of non-ANSI string functions provided by CRTDLL.
 */

/* NOTE: _wcscmpi not provided by CRTDLL, this define is for portability */
#define		_wcscmpi	_wcsicmp

_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW _wcsdup (const wchar_t*);
_CRTIMP int __cdecl __MINGW_NOTHROW	_wcsicmp (const wchar_t*, const wchar_t*);
_CRTIMP int __cdecl __MINGW_NOTHROW	_wcsicoll (const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW _wcslwr (wchar_t*);
_CRTIMP int __cdecl __MINGW_NOTHROW	_wcsnicmp (const wchar_t*, const wchar_t*, size_t);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW _wcsnset (wchar_t*, wchar_t, size_t);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW _wcsrev (wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW _wcsset (wchar_t*, wchar_t);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW _wcsupr (wchar_t*);

#ifdef __MSVCRT__
_CRTIMP int __cdecl __MINGW_NOTHROW  _wcsncoll(const wchar_t*, const wchar_t*, size_t);
_CRTIMP int   __cdecl __MINGW_NOTHROW _wcsnicoll(const wchar_t*, const wchar_t*, size_t);
#if __MSVCRT_VERSION__ >= 0x0700
_CRTIMP  wchar_t* __cdecl __MINGW_NOTHROW _wcserror(int);
_CRTIMP  wchar_t* __cdecl __MINGW_NOTHROW __wcserror(const wchar_t*);
#endif
#endif

#ifndef	_NO_OLDNAMES
/* NOTE: There is no _wcscmpi, but this is for compatibility. */
int __cdecl __MINGW_NOTHROW wcscmpi (const wchar_t * __ws1, const wchar_t * __ws2);
__CRT_INLINE int __cdecl __MINGW_NOTHROW
wcscmpi (const wchar_t * __ws1, const wchar_t * __ws2)
  {return _wcsicmp (__ws1, __ws2);}
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcsdup (const wchar_t*);
_CRTIMP int __cdecl __MINGW_NOTHROW	wcsicmp (const wchar_t*, const wchar_t*);
_CRTIMP int __cdecl __MINGW_NOTHROW	wcsicoll (const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcslwr (wchar_t*);
_CRTIMP int __cdecl __MINGW_NOTHROW	wcsnicmp (const wchar_t*, const wchar_t*, size_t);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcsnset (wchar_t*, wchar_t, size_t);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcsrev (wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcsset (wchar_t*, wchar_t);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW wcsupr (wchar_t*);
#endif	/* Not _NO_OLDNAMES */

#endif	/* Not strict ANSI */

#define _WSTRING_DEFINED
#endif  /* _WSTRING_DEFINED */

#ifdef __cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _STRING_H_ */
