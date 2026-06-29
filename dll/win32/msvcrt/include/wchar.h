/*
 * Unicode definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_WCHAR_H
#define __WINE_WCHAR_H

#include <corecrt_wctype.h>
#include <corecrt_wdirect.h>
#include <corecrt_wio.h>
#include <corecrt_wprocess.h>
#include <corecrt_wstdio.h>
#include <corecrt_wstdlib.h>
#include <corecrt_wstring.h>
#include <corecrt_wtime.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WCHAR_MIN  /* also in stdint.h */
#define WCHAR_MIN 0U
#define WCHAR_MAX 0xffffU
#endif

typedef int mbstate_t;

#ifndef _WLOCALE_DEFINED
#define _WLOCALE_DEFINED
_ACRTIMP wchar_t* __cdecl _wsetlocale(int,const wchar_t*);
#endif /* _WLOCALE_DEFINED */

wchar_t __cdecl btowc(int);
size_t  __cdecl mbrlen(const char *,size_t,mbstate_t*);
size_t  __cdecl mbrtowc(wchar_t*,const char*,size_t,mbstate_t*);
size_t  __cdecl mbsrtowcs(wchar_t*,const char**,size_t,mbstate_t*);
size_t  __cdecl wcrtomb(char*,wchar_t,mbstate_t*);
int     __cdecl wcrtomb_s(size_t*,char*,size_t,wchar_t,mbstate_t*);
size_t  __cdecl wcsrtombs(char*,const wchar_t**,size_t,mbstate_t*);
int     __cdecl wctob(wint_t);

_ACRTIMP errno_t __cdecl wmemcpy_s(wchar_t *, size_t, const wchar_t *, size_t);

static inline wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n)
{
    const wchar_t *end;
    for (end = s + n; s < end; s++)
        if (*s == c) return (wchar_t*)s;
    return NULL;
}

static inline int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
    {
        if (s1[i] > s2[i]) return 1;
        if (s1[i] < s2[i]) return -1;
    }
    return 0;
}

static inline wchar_t* __cdecl wmemcpy(wchar_t *dst, const wchar_t *src, size_t n)
{
    return (wchar_t*)memcpy(dst, src, n * sizeof(wchar_t));
}

static inline wchar_t* __cdecl wmemmove(wchar_t *dst, const wchar_t *src, size_t n)
{
    return (wchar_t*)memmove(dst, src, n * sizeof(wchar_t));
}

static inline wchar_t* __cdecl wmemset(wchar_t *s, wchar_t c, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        s[i] = c;
    return s;
}

#ifdef __cplusplus
}
#endif

#endif /* __WINE_WCHAR_H */
