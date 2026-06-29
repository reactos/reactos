/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _WSTDLIB_DEFINED
#define _WSTDLIB_DEFINED

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP wchar_t*      __cdecl _itow(int,wchar_t*,int);
_ACRTIMP errno_t       __cdecl _itow_s(int,wchar_t*,size_t, int);
_ACRTIMP wchar_t*      __cdecl _i64tow(__int64,wchar_t*,int);
_ACRTIMP errno_t       __cdecl _i64tow_s(__int64, wchar_t*, size_t, int);
_ACRTIMP wchar_t*      __cdecl _ltow(__msvcrt_long,wchar_t*,int);
_ACRTIMP errno_t       __cdecl _ltow_s(__msvcrt_long,wchar_t*,size_t,int);
_ACRTIMP wchar_t*      __cdecl _ui64tow(unsigned __int64,wchar_t*,int);
_ACRTIMP errno_t       __cdecl _ui64tow_s(unsigned __int64, wchar_t*, size_t, int);
_ACRTIMP wchar_t*      __cdecl _ultow(__msvcrt_ulong,wchar_t*,int);
_ACRTIMP errno_t       __cdecl _ultow_s(__msvcrt_ulong, wchar_t*, size_t, int);
_ACRTIMP wchar_t*      __cdecl _wfullpath(wchar_t*,const wchar_t*,size_t);
_ACRTIMP wchar_t*      __cdecl _wgetenv(const wchar_t*);
_ACRTIMP errno_t       __cdecl _wgetenv_s(size_t *,wchar_t *,size_t,const wchar_t *);
_ACRTIMP void          __cdecl _wmakepath(wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*);
_ACRTIMP int           __cdecl _wmakepath_s(wchar_t*,size_t,const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*);
_ACRTIMP void          __cdecl _wperror(const wchar_t*);
_ACRTIMP int           __cdecl _wputenv(const wchar_t*);
_ACRTIMP void          __cdecl _wsearchenv(const wchar_t*,const wchar_t*,wchar_t*);
_ACRTIMP void          __cdecl _wsplitpath(const wchar_t*,wchar_t*,wchar_t*,wchar_t*,wchar_t*);
_ACRTIMP errno_t       __cdecl _wsplitpath_s(const wchar_t*,wchar_t*,size_t,wchar_t*,size_t,
                                             wchar_t*,size_t,wchar_t*,size_t);
_ACRTIMP int           __cdecl _wsystem(const wchar_t*);
_ACRTIMP double        __cdecl _wtof(const wchar_t*);
_ACRTIMP int           __cdecl _wtoi(const wchar_t*);
_ACRTIMP __int64       __cdecl _wtoi64(const wchar_t*);
_ACRTIMP __msvcrt_long __cdecl _wtol(const wchar_t*);

_ACRTIMP size_t        __cdecl mbstowcs(wchar_t*,const char*,size_t);
_ACRTIMP size_t        __cdecl _mbstowcs_l(wchar_t*,const char*,size_t,_locale_t);
_ACRTIMP errno_t       __cdecl mbstowcs_s(size_t*,wchar_t*,size_t,const char*,size_t);
_ACRTIMP errno_t       __cdecl _mbstowcs_s_l(size_t*,wchar_t*,size_t,const char*,size_t,_locale_t);
_ACRTIMP int           __cdecl mbtowc(wchar_t*,const char*,size_t);
_ACRTIMP int           __cdecl _mbtowc_l(wchar_t*,const char*,size_t,_locale_t);
_ACRTIMP float         __cdecl wcstof(const wchar_t*,wchar_t**);
_ACRTIMP float         __cdecl _wcstof_l(const wchar_t*,wchar_t**,_locale_t);
_ACRTIMP double        __cdecl wcstod(const wchar_t*,wchar_t**);
_ACRTIMP __msvcrt_long __cdecl wcstol(const wchar_t*,wchar_t**,int);
_ACRTIMP size_t        __cdecl wcstombs(char*,const wchar_t*,size_t);
_ACRTIMP size_t        __cdecl _wcstombs_l(char*,const wchar_t*,size_t,_locale_t);
_ACRTIMP errno_t       __cdecl wcstombs_s(size_t*,char*,size_t,const wchar_t*,size_t);
_ACRTIMP __msvcrt_ulong __cdecl wcstoul(const wchar_t*,wchar_t**,int);
_ACRTIMP int           __cdecl wctomb(char*,wchar_t);
_ACRTIMP int           __cdecl _wctomb_l(char*,wchar_t,_locale_t);
_ACRTIMP __int64       __cdecl _wcstoi64(const wchar_t*,wchar_t**,int);
_ACRTIMP __int64       __cdecl _wcstoi64_l(const wchar_t*,wchar_t**,int,_locale_t);
_ACRTIMP unsigned __int64 __cdecl _wcstoui64(const wchar_t*,wchar_t**,int);
_ACRTIMP unsigned __int64 __cdecl _wcstoui64_l(const wchar_t*,wchar_t**,int,_locale_t);
_ACRTIMP __int64       __cdecl wcstoll(const wchar_t*,wchar_t**,int);
_ACRTIMP __int64       __cdecl _wcstoll_l(const wchar_t*,wchar_t**,int,_locale_t);
_ACRTIMP unsigned __int64 __cdecl wcstoull(const wchar_t*,wchar_t**,int);
_ACRTIMP unsigned __int64 __cdecl _wcstoull_l(const wchar_t*,wchar_t**,int,_locale_t);

#ifdef _UCRT
_ACRTIMP double __cdecl _wcstold_l(const wchar_t*,wchar_t**,_locale_t);
static inline long double wcstold(const wchar_t *string, wchar_t **endptr) { return _wcstold_l(string, endptr, NULL); }
#endif /* _UCRT */

#ifdef __cplusplus
extern "C++" {

template <size_t size>
inline errno_t _wgetenv_s(size_t *ret, wchar_t (&buf)[size], const wchar_t *var)
{
    return _wgetenv_s(ret, buf, size, var);
}

} /* extern "C++" */

}
#endif

#endif /* _WSTDLIB_DEFINED */
