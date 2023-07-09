/***
*string.h - declarations for string manipulation functions
*
*       Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the function declarations for the string
*       manipulation functions.
*       [ANSI/System V]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_STRING
#define _INC_STRING

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else   /* ndef _NTSDK */
/* current definition */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* _NTSDK */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif


#ifndef _MAC
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif /* ndef _MAC */

#ifndef _NLSCMP_DEFINED
#define _NLSCMPERROR    2147483647  /* currently == INT_MAX */
#define _NLSCMP_DEFINED
#endif

/* Define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


/* Function prototypes */

#ifdef _M_MRX000
_CRTIMP void *  __cdecl memcpy(void *, const void *, size_t);
_CRTIMP int     __cdecl memcmp(const void *, const void *, size_t);
_CRTIMP void *  __cdecl memset(void *, int, size_t);
_CRTIMP char *  __cdecl _strset(char *, int);
_CRTIMP char *  __cdecl strcpy(char *, const char *);
_CRTIMP char *  __cdecl strcat(char *, const char *);
_CRTIMP int     __cdecl strcmp(const char *, const char *);
_CRTIMP size_t  __cdecl strlen(const char *);
#else
        void *  __cdecl memcpy(void *, const void *, size_t);
        int     __cdecl memcmp(const void *, const void *, size_t);
        void *  __cdecl memset(void *, int, size_t);
        char *  __cdecl _strset(char *, int);
        char *  __cdecl strcpy(char *, const char *);
        char *  __cdecl strcat(char *, const char *);
        int     __cdecl strcmp(const char *, const char *);
        size_t  __cdecl strlen(const char *);
#endif
_CRTIMP void *  __cdecl _memccpy(void *, const void *, int, unsigned int);
_CRTIMP void *  __cdecl memchr(const void *, int, size_t);
_CRTIMP int     __cdecl _memicmp(const void *, const void *, unsigned int);

#ifdef  _M_ALPHA
        /* memmove is available as an intrinsic in the Alpha compiler */
        void *  __cdecl memmove(void *, const void *, size_t);
#else
_CRTIMP void *  __cdecl memmove(void *, const void *, size_t);
#endif


_CRTIMP char *  __cdecl strchr(const char *, int);
_CRTIMP int     __cdecl _strcmpi(const char *, const char *);
_CRTIMP int     __cdecl _stricmp(const char *, const char *);
_CRTIMP int     __cdecl strcoll(const char *, const char *);
_CRTIMP int     __cdecl _stricoll(const char *, const char *);
_CRTIMP int     __cdecl _strncoll(const char *, const char *, size_t);
_CRTIMP int     __cdecl _strnicoll(const char *, const char *, size_t);
_CRTIMP size_t  __cdecl strcspn(const char *, const char *);
_CRTIMP char *  __cdecl _strdup(const char *);
_CRTIMP char *  __cdecl _strerror(const char *);
_CRTIMP char *  __cdecl strerror(int);
_CRTIMP char *  __cdecl _strlwr(char *);
_CRTIMP char *  __cdecl strncat(char *, const char *, size_t);
_CRTIMP int     __cdecl strncmp(const char *, const char *, size_t);
_CRTIMP int     __cdecl _strnicmp(const char *, const char *, size_t);
_CRTIMP char *  __cdecl strncpy(char *, const char *, size_t);
_CRTIMP char *  __cdecl _strnset(char *, int, size_t);
_CRTIMP char *  __cdecl strpbrk(const char *, const char *);
_CRTIMP char *  __cdecl strrchr(const char *, int);
_CRTIMP char *  __cdecl _strrev(char *);
_CRTIMP size_t  __cdecl strspn(const char *, const char *);
_CRTIMP char *  __cdecl strstr(const char *, const char *);
_CRTIMP char *  __cdecl strtok(char *, const char *);
_CRTIMP char *  __cdecl _strupr(char *);
_CRTIMP size_t  __cdecl strxfrm (char *, const char *, size_t);

#if defined(_M_MPPC) || defined(_M_M68K)
unsigned char * __cdecl _c2pstr(char *);
char * __cdecl _p2cstr(unsigned char *);

#if     !__STDC__
__inline unsigned char * __cdecl c2pstr(char *sz) { return _c2pstr(sz);};
__inline char * __cdecl p2cstr(unsigned char *sz) { return _p2cstr(sz);};
#endif
#endif

#if     !__STDC__

#ifdef  _NTSDK

/* Non-ANSI names for compatibility */
#define memccpy  _memccpy
#define memicmp  _memicmp
#define strcmpi  _strcmpi
#define stricmp  _stricmp
#define strdup   _strdup
#define strlwr   _strlwr
#define strnicmp _strnicmp
#define strnset  _strnset
#define strrev   _strrev
#define strset   _strset
#define strupr   _strupr
#define stricoll _stricoll
#else   /* ndef _NTSDK */

/* prototypes for oldnames.lib functions */
_CRTIMP void * __cdecl memccpy(void *, const void *, int, unsigned int);
_CRTIMP int __cdecl memicmp(const void *, const void *, unsigned int);
_CRTIMP int __cdecl strcmpi(const char *, const char *);
_CRTIMP int __cdecl stricmp(const char *, const char *);
_CRTIMP char * __cdecl strdup(const char *);
_CRTIMP char * __cdecl strlwr(char *);
_CRTIMP int __cdecl strnicmp(const char *, const char *, size_t);
_CRTIMP char * __cdecl strnset(char *, int, size_t);
_CRTIMP char * __cdecl strrev(char *);
        char * __cdecl strset(char *, int);
_CRTIMP char * __cdecl strupr(char *);

#endif  /* ndef _NTSDK */

#endif  /* !__STDC__ */


#ifndef _MAC
#ifndef _WSTRING_DEFINED

/* wide function prototypes, also declared in wchar.h  */

_CRTIMP wchar_t * __cdecl wcscat(wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl wcschr(const wchar_t *, wchar_t);
_CRTIMP int __cdecl wcscmp(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl wcscpy(wchar_t *, const wchar_t *);
_CRTIMP size_t __cdecl wcscspn(const wchar_t *, const wchar_t *);
_CRTIMP size_t __cdecl wcslen(const wchar_t *);
_CRTIMP wchar_t * __cdecl wcsncat(wchar_t *, const wchar_t *, size_t);
_CRTIMP int __cdecl wcsncmp(const wchar_t *, const wchar_t *, size_t);
_CRTIMP wchar_t * __cdecl wcsncpy(wchar_t *, const wchar_t *, size_t);
_CRTIMP wchar_t * __cdecl wcspbrk(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl wcsrchr(const wchar_t *, wchar_t);
_CRTIMP size_t __cdecl wcsspn(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl wcsstr(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl wcstok(wchar_t *, const wchar_t *);

_CRTIMP wchar_t * __cdecl _wcsdup(const wchar_t *);
_CRTIMP int __cdecl _wcsicmp(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wcsnicmp(const wchar_t *, const wchar_t *, size_t);
_CRTIMP wchar_t * __cdecl _wcsnset(wchar_t *, wchar_t, size_t);
_CRTIMP wchar_t * __cdecl _wcsrev(wchar_t *);
_CRTIMP wchar_t * __cdecl _wcsset(wchar_t *, wchar_t);

_CRTIMP wchar_t * __cdecl _wcslwr(wchar_t *);
_CRTIMP wchar_t * __cdecl _wcsupr(wchar_t *);
_CRTIMP size_t __cdecl wcsxfrm(wchar_t *, const wchar_t *, size_t);
_CRTIMP int __cdecl wcscoll(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wcsicoll(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wcsncoll(const wchar_t *, const wchar_t *, size_t);
_CRTIMP int __cdecl _wcsnicoll(const wchar_t *, const wchar_t *, size_t);

#if     !__STDC__

/* old names */
#define wcswcs wcsstr

#ifdef  _NTSDK

/* Non-ANSI names for compatibility */
#define wcsdup  _wcsdup
#define wcsicmp _wcsicmp
#define wcsnicmp _wcsnicmp
#define wcsnset _wcsnset
#define wcsrev  _wcsrev
#define wcsset  _wcsset
#define wcslwr  _wcslwr
#define wcsupr  _wcsupr
#define wcsicoll _wcsicoll

#else   /* ndef _NTSDK */

/* prototypes for oldnames.lib functions */
_CRTIMP wchar_t * __cdecl wcsdup(const wchar_t *);
_CRTIMP int __cdecl wcsicmp(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl wcsnicmp(const wchar_t *, const wchar_t *, size_t);
_CRTIMP wchar_t * __cdecl wcsnset(wchar_t *, wchar_t, size_t);
_CRTIMP wchar_t * __cdecl wcsrev(wchar_t *);
_CRTIMP wchar_t * __cdecl wcsset(wchar_t *, wchar_t);
_CRTIMP wchar_t * __cdecl wcslwr(wchar_t *);
_CRTIMP wchar_t * __cdecl wcsupr(wchar_t *);
_CRTIMP int __cdecl wcsicoll(const wchar_t *, const wchar_t *);

#endif  /* ndef _NTSDK */

#endif  /* !__STDC__ */

#define _WSTRING_DEFINED
#endif

#endif /* ndef _MAC */

#ifdef __cplusplus
}
#endif

#endif  /* _INC_STRING */
