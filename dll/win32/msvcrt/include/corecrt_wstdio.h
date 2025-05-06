/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifndef _WSTDIO_DEFINED
#define _WSTDIO_DEFINED

#include <corecrt.h>
#include <corecrt_stdio_config.h>

#ifndef RC_INVOKED
#include <stdarg.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <pshpack8.h>

#ifndef _FILE_DEFINED
#define _FILE_DEFINED
#include <pshpack8.h>
typedef struct _iobuf
{
#ifdef _UCRT
  void *_Placeholder;
#else
  char* _ptr;
  int   _cnt;
  char* _base;
  int   _flag;
  int   _file;
  int   _charbuf;
  int   _bufsiz;
  char* _tmpfname;
#endif
} FILE;
#include <poppack.h>
#endif  /* _FILE_DEFINED */

#ifndef WEOF
#define WEOF        (wint_t)(0xFFFF)
#endif

_ACRTIMP FILE *__cdecl __acrt_iob_func(unsigned index);

#define stdin  (__acrt_iob_func(0))
#define stdout (__acrt_iob_func(1))
#define stderr (__acrt_iob_func(2))

_ACRTIMP wint_t   __cdecl _fgetwc_nolock(FILE*);
_ACRTIMP wint_t   __cdecl _fgetwchar(void);
_ACRTIMP wint_t   __cdecl _fputwc_nolock(wint_t,FILE*);
_ACRTIMP wint_t   __cdecl _fputwchar(wint_t);
_ACRTIMP wint_t   __cdecl _getwc_nolock(FILE*);
_ACRTIMP wchar_t* __cdecl _getws(wchar_t*);
_ACRTIMP wint_t   __cdecl _putwc_nolock(wint_t,FILE*);
_ACRTIMP int      __cdecl _putws(const wchar_t*);
_ACRTIMP wint_t   __cdecl _ungetwc_nolock(wint_t,FILE*);
_ACRTIMP FILE*    __cdecl _wfdopen(int,const wchar_t*);
_ACRTIMP FILE*    __cdecl _wfopen(const wchar_t*,const wchar_t*);
_ACRTIMP errno_t  __cdecl _wfopen_s(FILE**,const wchar_t*,const wchar_t*);
_ACRTIMP FILE*    __cdecl _wfreopen(const wchar_t*,const wchar_t*,FILE*);
_ACRTIMP FILE*    __cdecl _wfsopen(const wchar_t*,const wchar_t*,int);
_ACRTIMP void     __cdecl _wperror(const wchar_t*);
_ACRTIMP FILE*    __cdecl _wpopen(const wchar_t*,const wchar_t*);
_ACRTIMP int      __cdecl _wremove(const wchar_t*);
_ACRTIMP wchar_t* __cdecl _wtempnam(const wchar_t*,const wchar_t*);
_ACRTIMP wchar_t* __cdecl _wtmpnam(wchar_t*);

_ACRTIMP wint_t   __cdecl fgetwc(FILE*);
_ACRTIMP wchar_t* __cdecl fgetws(wchar_t*,int,FILE*);
_ACRTIMP wint_t   __cdecl fputwc(wint_t,FILE*);
_ACRTIMP int      __cdecl fputws(const wchar_t*,FILE*);
_ACRTIMP int      __cdecl fputws(const wchar_t*,FILE*);
_ACRTIMP wint_t   __cdecl getwc(FILE*);
_ACRTIMP wint_t   __cdecl getwchar(void);
_ACRTIMP wchar_t* __cdecl getws(wchar_t*);
_ACRTIMP wint_t   __cdecl putwc(wint_t,FILE*);
_ACRTIMP wint_t   __cdecl putwchar(wint_t);
_ACRTIMP int      __cdecl putws(const wchar_t*);
_ACRTIMP wint_t   __cdecl ungetwc(wint_t,FILE*);

#ifdef _UCRT

_ACRTIMP int __cdecl __stdio_common_vfwprintf(unsigned __int64,FILE*,const wchar_t*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vfwprintf_s(unsigned __int64,FILE*,const wchar_t*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vsnwprintf_s(unsigned __int64,wchar_t*,size_t,size_t,const wchar_t*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vswprintf(unsigned __int64,wchar_t*,size_t,const wchar_t*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vswprintf_p(unsigned __int64,wchar_t*,size_t,const wchar_t*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vswprintf_s(unsigned __int64,wchar_t*,size_t,const wchar_t*,_locale_t,va_list);

_ACRTIMP int __cdecl __stdio_common_vfwscanf(unsigned __int64,FILE*,const wchar_t*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vswscanf(unsigned __int64,const wchar_t*,size_t,const wchar_t*,_locale_t,va_list);

#endif /* _UCRT */

#if defined(_UCRT) && !defined(_NO_CRT_STDIO_INLINE)

static inline int __cdecl _vsnwprintf(wchar_t *buffer, size_t size, const wchar_t *format, va_list args)
{
    int ret = __stdio_common_vswprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl _snwprintf(wchar_t *buffer, size_t size, const wchar_t* format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vswprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl _vsnwprintf_s(wchar_t *buffer, size_t size, size_t count, const wchar_t *format, va_list args)
{
    int ret = __stdio_common_vsnwprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, count, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl _snwprintf_s(wchar_t *buffer, size_t size, size_t count, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vsnwprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, count, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl vswprintf(wchar_t *buffer, size_t size, const wchar_t *format, va_list args)
{
    int ret = __stdio_common_vswprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl swprintf(wchar_t *buffer, size_t size, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vswprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl _vswprintf(wchar_t *buffer, const wchar_t *format, va_list args)
{
    int ret = __stdio_common_vswprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, -1, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl _swprintf(wchar_t *buffer, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vswprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, -1, format, NULL, args);
    va_end(args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl vswprintf_s(wchar_t *buffer, size_t size, const wchar_t *format, va_list args)
{
    int ret = __stdio_common_vswprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl swprintf_s(wchar_t *buffer, size_t size, const wchar_t* format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vswprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl _swprintf_l(wchar_t *buffer, size_t size, const wchar_t* format, _locale_t locale, ...)
{
    int ret;
    va_list args;

    va_start(args, locale);
    ret = __stdio_common_vswprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, locale, args);
    va_end(args);
    return ret;
}

static inline int __cdecl _vscwprintf(const wchar_t *format, va_list args)
{
    int ret = __stdio_common_vswprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
                                       NULL, 0, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl _vswprintf_p_l(wchar_t *buffer, size_t size, const wchar_t *format, _locale_t locale, va_list args)
{
    int ret = __stdio_common_vswprintf_p(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, locale, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl _vscwprintf_p_l(const wchar_t *format, _locale_t locale, va_list args)
{
    int ret = __stdio_common_vswprintf_p(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
                                         NULL, 0, format, locale, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl _scwprintf(const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vswprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
                                   NULL, 0, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl vfwprintf(FILE *file, const wchar_t *format, va_list args)
{
    return __stdio_common_vfwprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, file, format, NULL, args);
}

static inline int __cdecl fwprintf(FILE *file, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfwprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, file, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl vfwprintf_s(FILE *file, const wchar_t *format, va_list args)
{
    return __stdio_common_vfwprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, file, format, NULL, args);
}

static inline int __cdecl fwprintf_s(FILE *file, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = vfwprintf_s(file, format, args);
    va_end(args);
    return ret;
}

static inline int __cdecl vwprintf(const wchar_t *format, va_list args)
{
    return __stdio_common_vfwprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, stdout, format, NULL, args);
}

static inline int __cdecl wprintf(const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfwprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, stdout, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl vwprintf_s(const wchar_t *format, va_list args)
{
    return __stdio_common_vfwprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, stdout, format, NULL, args);
}

static inline int __cdecl wprintf_s(const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = vfwprintf_s(stdout, format, args);
    va_end(args);
    return ret;
}

static inline int __cdecl swscanf(const wchar_t *buffer, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vswscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS, buffer, -1, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl swscanf_s(const wchar_t *buffer, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vswscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT, buffer, -1, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl fwscanf(FILE *file, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfwscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS, file, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl fwscanf_s(FILE *file, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfwscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT, file, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl wscanf(FILE *file, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfwscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS, stdin, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl wscanf_s(FILE *file, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfwscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT, stdin, format, NULL, args);
    va_end(args);
    return ret;
}

#else /* _UCRT && !_NO_CRT_STDIO_INLINE */

_ACRTIMP int __cdecl _scwprintf(const wchar_t*,...);
_ACRTIMP int __cdecl _snwprintf(wchar_t*,size_t,const wchar_t*,...);
_ACRTIMP int __cdecl _snwprintf_s(wchar_t*,size_t,size_t,const wchar_t*,...);
_ACRTIMP int __cdecl _vscwprintf(const wchar_t*,va_list);
_ACRTIMP int __cdecl _vscwprintf_p_l(const wchar_t*,_locale_t,va_list);
_ACRTIMP int __cdecl _vsnwprintf(wchar_t*,size_t,const wchar_t*,va_list);
_ACRTIMP int __cdecl _vsnwprintf_s(wchar_t*,size_t,size_t,const wchar_t*,va_list);
_ACRTIMP int __cdecl _vswprintf_p_l(wchar_t*,size_t,const wchar_t*,_locale_t,va_list);
_ACRTIMP int __cdecl fwprintf(FILE*,const wchar_t*,...);
_ACRTIMP int __cdecl fwprintf_s(FILE*,const wchar_t*,...);
_ACRTIMP int __cdecl swprintf_s(wchar_t*,size_t,const wchar_t*,...);
_ACRTIMP int __cdecl vfwprintf(FILE*,const wchar_t*,va_list);
_ACRTIMP int __cdecl vfwprintf_s(FILE*,const wchar_t*,va_list);
_ACRTIMP int __cdecl vswprintf_s(wchar_t*,size_t,const wchar_t*,va_list);
_ACRTIMP int __cdecl vwprintf(const wchar_t*,va_list);
_ACRTIMP int __cdecl vwprintf_s(const wchar_t*,va_list);
_ACRTIMP int __cdecl wprintf(const wchar_t*,...);
_ACRTIMP int __cdecl wprintf_s(const wchar_t*,...);

#ifdef _CRT_NON_CONFORMING_SWPRINTFS
_ACRTIMP int __cdecl swprintf(wchar_t*,const wchar_t*,...);
_ACRTIMP int __cdecl vswprintf(wchar_t*,const wchar_t*,va_list);
#elif !defined(_NO_CRT_STDIO_INLINE)
static inline int __cdecl vswprintf(wchar_t *buffer, size_t size, const wchar_t *format, va_list args) { return _vsnwprintf(buffer,size,format,args); }
static inline int __cdecl swprintf(wchar_t *buffer, size_t size, const wchar_t *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = _vsnwprintf(buffer, size, format, args);
    va_end(args);
    return ret;
}
#else
_ACRTIMP int __cdecl vswprintf(wchar_t*,size_t,const wchar_t*,va_list);
_ACRTIMP int __cdecl swprintf(wchar_t*,size_t,const wchar_t*,...);
#endif  /*  _CRT_NON_CONFORMING_SWPRINTFS */

_ACRTIMP int __cdecl fwscanf(FILE*,const wchar_t*,...);
_ACRTIMP int __cdecl fwscanf_s(FILE*,const wchar_t*,...);
_ACRTIMP int __cdecl swscanf(const wchar_t*,const wchar_t*,...);
_ACRTIMP int __cdecl swscanf_s(const wchar_t*,const wchar_t*,...);
_ACRTIMP int __cdecl wscanf(const wchar_t*,...);
_ACRTIMP int __cdecl wscanf_s(const wchar_t*,...);

#endif /* _UCRT && !_NO_CRT_STDIO_INLINE */

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* _WSTDIO_DEFINED */
