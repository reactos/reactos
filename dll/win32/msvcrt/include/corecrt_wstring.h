/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _WSTRING_DEFINED
#define _WSTRING_DEFINED

#include <corecrt.h>
#include <corecrt_malloc.h>

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

_ACRTIMP void*   __cdecl memmove(void*,const void*,size_t);

_ACRTIMP wchar_t* __cdecl _wcsdup(const wchar_t*) __WINE_DEALLOC(free) __WINE_MALLOC;
_ACRTIMP int      __cdecl _wcsicmp(const wchar_t*,const wchar_t*);
_ACRTIMP int      __cdecl _wcsicmp_l(const wchar_t*,const wchar_t*, _locale_t);
_ACRTIMP int      __cdecl _wcsicoll(const wchar_t*,const wchar_t*);
_ACRTIMP int      __cdecl _wcsicoll_l(const wchar_t*, const wchar_t*, _locale_t);
_ACRTIMP wchar_t* __cdecl _wcslwr(wchar_t*);
_ACRTIMP errno_t  __cdecl _wcslwr_s(wchar_t*, size_t);
_ACRTIMP int      __cdecl _wcscoll_l(const wchar_t*, const wchar_t*, _locale_t);
_ACRTIMP int      __cdecl _wcsncoll(const wchar_t*, const wchar_t*, size_t);
_ACRTIMP int      __cdecl _wcsncoll_l(const wchar_t*, const wchar_t*, size_t, _locale_t);
_ACRTIMP int      __cdecl _wcsnicmp(const wchar_t*,const wchar_t*,size_t);
_ACRTIMP int      __cdecl _wcsnicoll(const wchar_t*,const wchar_t*,size_t);
_ACRTIMP int      __cdecl _wcsnicoll_l(const wchar_t*, const wchar_t*, size_t, _locale_t);
_ACRTIMP size_t   __cdecl _wcsnlen(const wchar_t*,size_t);
_ACRTIMP wchar_t* __cdecl _wcsnset(wchar_t*,wchar_t,size_t);
_ACRTIMP wchar_t* __cdecl _wcsrev(wchar_t*);
_ACRTIMP wchar_t* __cdecl _wcsset(wchar_t*,wchar_t);
_ACRTIMP wchar_t* __cdecl _wcsupr(wchar_t*);
_ACRTIMP errno_t  __cdecl _wcsupr_s(wchar_t*, size_t);
_ACRTIMP size_t   __cdecl _wcsxfrm_l(wchar_t*,const wchar_t*,size_t,_locale_t);

_ACRTIMP wchar_t* __cdecl wcscat(wchar_t*,const wchar_t*);
_ACRTIMP errno_t  __cdecl wcscat_s(wchar_t*,size_t,const wchar_t*);
_ACRTIMP wchar_t* __cdecl wcschr(const wchar_t*,wchar_t);
_ACRTIMP int      __cdecl wcscmp(const wchar_t*,const wchar_t*);
_ACRTIMP int      __cdecl wcscoll(const wchar_t*,const wchar_t*);
_ACRTIMP wchar_t* __cdecl wcscpy(wchar_t*,const wchar_t*);
_ACRTIMP errno_t  __cdecl wcscpy_s(wchar_t*,size_t,const wchar_t*);
_ACRTIMP size_t   __cdecl wcscspn(const wchar_t*,const wchar_t*);
_ACRTIMP size_t   __cdecl wcslen(const wchar_t*);
_ACRTIMP wchar_t* __cdecl wcsncat(wchar_t*,const wchar_t*,size_t);
_ACRTIMP errno_t  __cdecl wcsncat_s(wchar_t*,size_t,const wchar_t*,size_t);
_ACRTIMP int      __cdecl wcsncmp(const wchar_t*,const wchar_t*,size_t);
_ACRTIMP wchar_t* __cdecl wcsncpy(wchar_t*,const wchar_t*,size_t);
_ACRTIMP errno_t  __cdecl wcsncpy_s(wchar_t*,size_t,const wchar_t*,size_t);
_ACRTIMP size_t   __cdecl wcsnlen(const wchar_t*,size_t);
_ACRTIMP wchar_t* __cdecl wcspbrk(const wchar_t*,const wchar_t*);
_ACRTIMP wchar_t* __cdecl wcsrchr(const wchar_t*,wchar_t wcFor);
_ACRTIMP size_t   __cdecl wcsspn(const wchar_t*,const wchar_t*);
_ACRTIMP wchar_t* __cdecl wcsstr(const wchar_t*,const wchar_t*);
_ACRTIMP wchar_t* __cdecl wcstok_s(wchar_t*,const wchar_t*,wchar_t**);
_ACRTIMP size_t   __cdecl wcsxfrm(wchar_t*,const wchar_t*,size_t);

#ifdef _UCRT
_ACRTIMP wchar_t* __cdecl wcstok(wchar_t*,const wchar_t*,wchar_t**);
static inline wchar_t* _wcstok(wchar_t* str, const wchar_t *delim) { return wcstok(str, delim, NULL); }
#  ifdef __cplusplus
extern "C++" inline wchar_t* wcstok(wchar_t* str, const wchar_t *delim) { return wcstok(str, delim, NULL); }
#  elif defined(_CRT_NON_CONFORMING_WCSTOK)
#    define wcstok _wcstok
#  endif
#else /* _UCRT */
_ACRTIMP wchar_t* __cdecl wcstok(wchar_t*,const wchar_t*);
#  define _wcstok wcstok
#endif /* _UCRT */

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C++" {
template <size_t S> inline errno_t wcscat_s(wchar_t (&dst)[S], const wchar_t *arg) throw() { return wcscat_s(dst, S, arg); }
} /* extern "C++" */
#endif /* __cplusplus */

#endif /* _WSTRING_DEFINED */
