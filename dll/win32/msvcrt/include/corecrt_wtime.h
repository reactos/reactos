/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _WTIME_DEFINED
#define _WTIME_DEFINED

#include <corecrt.h>

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

#if defined(_USE_32BIT_TIME_T) && !defined(_UCRT)
#define _wctime32 _wctime
#endif

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP wchar_t* __cdecl _wasctime(const struct tm*);
_ACRTIMP size_t   __cdecl wcsftime(wchar_t*,size_t,const wchar_t*,const struct tm*);
_ACRTIMP size_t   __cdecl _wcsftime_l(wchar_t*,size_t,const wchar_t*,const struct tm*,_locale_t);
_ACRTIMP wchar_t* __cdecl _wctime32(const __time32_t*);
_ACRTIMP wchar_t* __cdecl _wctime64(const __time64_t*);
_ACRTIMP wchar_t* __cdecl _wstrdate(wchar_t*);
_ACRTIMP errno_t  __cdecl _wstrdate_s(wchar_t*,size_t);
_ACRTIMP wchar_t* __cdecl _wstrtime(wchar_t*);
_ACRTIMP errno_t  __cdecl _wstrtime_s(wchar_t*,size_t);

#ifndef _USE_32BIT_TIME_T
static inline wchar_t* _wctime(const time_t *t) { return _wctime64(t); }
#elif defined(_UCRT)
static inline wchar_t* _wctime(const time_t *t) { return _wctime32(t); }
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WTIME_DEFINED */
