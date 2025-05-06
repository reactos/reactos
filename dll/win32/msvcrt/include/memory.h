/*
 * Memory definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_MEMORY_H
#define __WINE_MEMORY_H

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CRT_MEMORY_DEFINED
#define _CRT_MEMORY_DEFINED

_ACRTIMP void*   __cdecl memchr(const void*,int,size_t);
_ACRTIMP int     __cdecl memcmp(const void*,const void*,size_t);
_ACRTIMP void*   __cdecl memcpy(void*,const void*,size_t);
_ACRTIMP errno_t __cdecl memcpy_s(void*,size_t,const void*,size_t);
_ACRTIMP void*   __cdecl memset(void*,int,size_t);
_ACRTIMP void*   __cdecl _memccpy(void*,const void*,int,size_t);
_ACRTIMP int     __cdecl _memicmp(const void*,const void*,size_t);
_ACRTIMP int     __cdecl _memicmp_l(const void*,const void*,size_t,_locale_t);

static inline int memicmp(const void* s1, const void* s2, size_t len) { return _memicmp(s1, s2, len); }
static inline void* memccpy(void *s1, const void *s2, int c, size_t n) { return _memccpy(s1, s2, c, n); }

#endif /* _CRT_MEMORY_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* __WINE_MEMORY_H */
