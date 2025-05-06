/*
 * String definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STRING_H
#define __WINE_STRING_H

#include <corecrt_malloc.h>
#include <corecrt_wstring.h>

#ifndef _NLSCMP_DEFINED
#define _NLSCMPERROR               ((unsigned int)0x7fffffff)
#define _NLSCMP_DEFINED
#endif

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP size_t __cdecl __strncnt(const char*,size_t);
_ACRTIMP int   __cdecl _strcmpi(const char*,const char*);
_ACRTIMP int   __cdecl _strcoll_l(const char*, const char*, _locale_t);
_ACRTIMP char* __cdecl _strdup(const char*) __WINE_DEALLOC(free) __WINE_MALLOC;
_ACRTIMP char* __cdecl _strerror(const char*);
_ACRTIMP errno_t __cdecl strerror_s(char*,size_t,int);
_ACRTIMP int   __cdecl _stricmp(const char*,const char*);
_ACRTIMP int   __cdecl _stricoll(const char*,const char*);
_ACRTIMP int   __cdecl _stricoll_l(const char*, const char*, _locale_t);
_ACRTIMP char* __cdecl _strlwr(char*);
_ACRTIMP errno_t __cdecl _strlwr_s(char*,size_t);
_ACRTIMP int   __cdecl _strncoll(const char*, const char*, size_t);
_ACRTIMP int   __cdecl _strncoll_l(const char*, const char*, size_t, _locale_t);
_ACRTIMP int   __cdecl _strnicmp(const char*,const char*,size_t);
_ACRTIMP int   __cdecl _strnicmp_l(const char*, const char*, size_t, _locale_t);
_ACRTIMP int   __cdecl _strnicoll(const char*, const char*, size_t);
_ACRTIMP int   __cdecl _strnicoll_l(const char*, const char*, size_t, _locale_t);
_ACRTIMP char* __cdecl _strnset(char*,int,size_t);
_ACRTIMP char* __cdecl _strrev(char*);
_ACRTIMP char* __cdecl _strset(char*,int);
_ACRTIMP char* __cdecl _strupr(char*);
_ACRTIMP errno_t __cdecl _strupr_s(char *, size_t);
_ACRTIMP size_t  __cdecl _strxfrm_l(char*,const char*,size_t,_locale_t);

_ACRTIMP errno_t __cdecl memmove_s(void*,size_t,const void*,size_t);
_ACRTIMP char*   __cdecl strcat(char*,const char*);
_ACRTIMP errno_t __cdecl strcat_s(char*,size_t,const char*);
_ACRTIMP char*   __cdecl strchr(const char*,int);
_ACRTIMP int     __cdecl strcmp(const char*,const char*);
_ACRTIMP int     __cdecl strcoll(const char*,const char*);
_ACRTIMP char*   __cdecl strcpy(char*,const char*);
_ACRTIMP errno_t __cdecl strcpy_s(char*,size_t,const char*);
_ACRTIMP size_t  __cdecl strcspn(const char*,const char*);
_ACRTIMP char*   __cdecl strerror(int);
_ACRTIMP size_t  __cdecl strlen(const char*);
_ACRTIMP char*   __cdecl strncat(char*,const char*,size_t);
_ACRTIMP errno_t __cdecl strncat_s(char*,size_t,const char*,size_t);
_ACRTIMP int     __cdecl strncmp(const char*,const char*,size_t);
_ACRTIMP char*   __cdecl strncpy(char*,const char*,size_t);
_ACRTIMP errno_t __cdecl strncpy_s(char*,size_t,const char*,size_t);
_ACRTIMP size_t  __cdecl strnlen(const char*,size_t);
_ACRTIMP char*   __cdecl strpbrk(const char*,const char*);
_ACRTIMP char*   __cdecl strrchr(const char*,int);
_ACRTIMP size_t  __cdecl strspn(const char*,const char*);
_ACRTIMP char*   __cdecl strstr(const char*,const char*);
_ACRTIMP char*   __cdecl strtok(char*,const char*);
_ACRTIMP char*   __cdecl strtok_s(char*,const char*,char**);
_ACRTIMP size_t  __cdecl strxfrm(char*,const char*,size_t);

#ifdef __cplusplus
}
#endif


static inline int strcasecmp(const char* s1, const char* s2) { return _stricmp(s1, s2); }
static inline int strcmpi(const char* s1, const char* s2) { return _strcmpi(s1, s2); }
static inline char* strdup(const char* buf) { return _strdup(buf); }
static inline int stricmp(const char* s1, const char* s2) { return _stricmp(s1, s2); }
static inline int stricoll(const char* s1, const char* s2) { return _stricoll(s1, s2); }
static inline char* strlwr(char* str) { return _strlwr(str); }
static inline int strncasecmp(const char *str1, const char *str2, size_t n) { return _strnicmp(str1, str2, n); }
static inline int strnicmp(const char* s1, const char* s2, size_t n) { return _strnicmp(s1, s2, n); }
static inline char* strnset(char* str, int value, unsigned int len) { return _strnset(str, value, len); }
static inline char* strrev(char* str) { return _strrev(str); }
static inline char* strset(char* str, int value) { return _strset(str, value); }
static inline char* strupr(char* str) { return _strupr(str); }

static inline wchar_t* wcsdup(const wchar_t* str) { return _wcsdup(str); }
static inline int wcsicoll(const wchar_t* str1, const wchar_t* str2) { return _wcsicoll(str1, str2); }
static inline wchar_t* wcslwr(wchar_t* str) { return _wcslwr(str); }
static inline int wcsicmp(const wchar_t* s1, const wchar_t* s2) { return _wcsicmp(s1, s2); }
static inline int wcsnicmp(const wchar_t* str1, const wchar_t* str2, size_t n) { return _wcsnicmp(str1, str2, n); }
static inline wchar_t* wcsnset(wchar_t* str, wchar_t c, size_t n) { return _wcsnset(str, c, n); }
static inline wchar_t* wcsrev(wchar_t* str) { return _wcsrev(str); }
static inline wchar_t* wcsset(wchar_t* str, wchar_t c) { return _wcsset(str, c); }
static inline wchar_t* wcsupr(wchar_t* str) { return _wcsupr(str); }

#endif /* __WINE_STRING_H */
