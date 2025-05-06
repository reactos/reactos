/*
 * Standard I/O definitions.
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STDIO_H
#define __WINE_STDIO_H

#include <corecrt_wstdio.h>

/* file._flag flags */
#ifndef _UCRT
#define _IOREAD          0x0001
#define _IOWRT           0x0002
#define _IOMYBUF         0x0008
#define _IOEOF           0x0010
#define _IOERR           0x0020
#define _IOSTRG          0x0040
#define _IORW            0x0080
#endif

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* more file._flag flags, but these conflict with Unix */
#define _IOFBF    0x0000
#define _IONBF    0x0004
#define _IOLBF    0x0040

#define EOF       (-1)
#define FILENAME_MAX 260
#define TMP_MAX   0x7fff
#define TMP_MAX_S 0x7fffffff
#define FOPEN_MAX 20
#define L_tmpnam  260

#define BUFSIZ    512

#ifndef SEEK_SET
#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2
#endif

#ifndef _FPOS_T_DEFINED
typedef __int64 DECLSPEC_ALIGN(8) fpos_t;
#define _FPOS_T_DEFINED
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _STDIO_DEFINED
# ifdef __i386__
_ACRTIMP FILE* __cdecl __p__iob(void);
#  define _iob (__p__iob())
# else
_ACRTIMP FILE* __cdecl __iob_func(void);
#  define _iob (__iob_func())
# endif
#endif /* _STDIO_DEFINED */

/* return value for _get_output_format */
#define _TWO_DIGIT_EXPONENT 0x1

#ifndef _STDIO_DEFINED
#define _STDIO_DEFINED
_ACRTIMP int    __cdecl _fcloseall(void);
_ACRTIMP FILE*  __cdecl _fdopen(int,const char*);
_ACRTIMP int    __cdecl _fgetchar(void);
_ACRTIMP int    __cdecl _filbuf(FILE*);
_ACRTIMP int    __cdecl _fileno(FILE*);
_ACRTIMP int    __cdecl _flsbuf(int,FILE*);
_ACRTIMP int    __cdecl _flushall(void);
_ACRTIMP int    __cdecl _fputchar(int);
_ACRTIMP FILE*  __cdecl _fsopen(const char*,const char*,int);
_ACRTIMP int    __cdecl _get_printf_count_output(void);
_ACRTIMP int    __cdecl _getmaxstdio(void);
_ACRTIMP int    __cdecl _getw(FILE*);
_ACRTIMP int    __cdecl _pclose(FILE*);
_ACRTIMP FILE*  __cdecl _popen(const char*,const char*);
_ACRTIMP int    __cdecl _putw(int,FILE*);
_ACRTIMP int    __cdecl _rmtmp(void);
_ACRTIMP int    __cdecl _set_printf_count_output(int);
_ACRTIMP int    __cdecl _setmaxstdio(int);
_ACRTIMP char*  __cdecl _tempnam(const char*,const char*);
_ACRTIMP int    __cdecl _unlink(const char*);

_ACRTIMP void __cdecl _lock_file(FILE*);
_ACRTIMP void __cdecl _unlock_file(FILE*);
_ACRTIMP size_t __cdecl _fread_nolock(void*,size_t,size_t,FILE*);
_ACRTIMP size_t __cdecl _fread_nolock_s(void*,size_t,size_t,size_t,FILE*);
_ACRTIMP size_t __cdecl _fwrite_nolock(const void*,size_t,size_t,FILE*);
_ACRTIMP int    __cdecl _fclose_nolock(FILE*);
_ACRTIMP int    __cdecl _fflush_nolock(FILE*);
_ACRTIMP int    __cdecl _fgetc_nolock(FILE*);
_ACRTIMP int    __cdecl _fputc_nolock(int,FILE*);
_ACRTIMP int    __cdecl _fseek_nolock(FILE*,__msvcrt_long,int);
_ACRTIMP int    __cdecl _fseeki64_nolock(FILE*,__int64,int);
_ACRTIMP __msvcrt_long __cdecl _ftell_nolock(FILE*);
_ACRTIMP __int64 __cdecl _ftelli64_nolock(FILE*);
_ACRTIMP int    __cdecl _getc_nolock(FILE*);
_ACRTIMP int    __cdecl _putc_nolock(int,FILE*);
_ACRTIMP int    __cdecl _ungetc_nolock(int,FILE*);

_ACRTIMP void   __cdecl clearerr(FILE*);
_ACRTIMP errno_t __cdecl clearerr_s(FILE*);
_ACRTIMP int    __cdecl fclose(FILE*);
_ACRTIMP int    __cdecl feof(FILE*);
_ACRTIMP int    __cdecl ferror(FILE*);
_ACRTIMP int    __cdecl fflush(FILE*);
_ACRTIMP int    __cdecl fgetc(FILE*);
_ACRTIMP int    __cdecl fgetpos(FILE*,fpos_t*);
_ACRTIMP char*  __cdecl fgets(char*,int,FILE*);
_ACRTIMP FILE*  __cdecl fopen(const char*,const char*);
_ACRTIMP errno_t __cdecl fopen_s(FILE**,const char*,const char*);
_ACRTIMP int    __cdecl fputc(int,FILE*);
_ACRTIMP int    __cdecl fputs(const char*,FILE*);
_ACRTIMP size_t __cdecl fread(void*,size_t,size_t,FILE*);
_ACRTIMP size_t __cdecl fread_s(void*,size_t,size_t,size_t,FILE*);
_ACRTIMP FILE*  __cdecl freopen(const char*,const char*,FILE*);
_ACRTIMP errno_t __cdecl freopen_s(FILE**,const char*,const char*,FILE*);
_ACRTIMP int    __cdecl fseek(FILE*,__msvcrt_long,int);
_ACRTIMP int    __cdecl _fseeki64(FILE*,__int64,int);
_ACRTIMP int    __cdecl fsetpos(FILE*,fpos_t*);
_ACRTIMP __msvcrt_long __cdecl ftell(FILE*);
_ACRTIMP __int64 __cdecl _ftelli64(FILE*);
_ACRTIMP size_t __cdecl fwrite(const void*,size_t,size_t,FILE*);
_ACRTIMP int    __cdecl getc(FILE*);
_ACRTIMP int    __cdecl getchar(void);
_ACRTIMP char*  __cdecl gets(char*);
_ACRTIMP void   __cdecl perror(const char*);
_ACRTIMP int    __cdecl putc(int,FILE*);
_ACRTIMP int    __cdecl putchar(int);
_ACRTIMP int    __cdecl puts(const char*);
_ACRTIMP int    __cdecl remove(const char*);
_ACRTIMP int    __cdecl rename(const char*,const char*);
_ACRTIMP void   __cdecl rewind(FILE*);
_ACRTIMP void   __cdecl setbuf(FILE*,char*);
_ACRTIMP int    __cdecl setvbuf(FILE*,char*,int,size_t);
_ACRTIMP FILE*  __cdecl tmpfile(void);
_ACRTIMP char*  __cdecl tmpnam(char*);
_ACRTIMP int    __cdecl ungetc(int,FILE*);
_ACRTIMP unsigned int __cdecl _get_output_format(void);
_ACRTIMP unsigned int __cdecl _set_output_format(unsigned int);

#ifdef _UCRT

_ACRTIMP int __cdecl __stdio_common_vfprintf(unsigned __int64,FILE*,const char*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vfprintf_s(unsigned __int64,FILE*,const char*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vsprintf(unsigned __int64,char*,size_t,const char*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vsprintf_p(unsigned __int64,char*,size_t,const char*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vsprintf_s(unsigned __int64,char*,size_t,const char*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vsnprintf_s(unsigned __int64,char*,size_t,size_t,const char*,_locale_t,va_list);

_ACRTIMP int __cdecl __stdio_common_vfscanf(unsigned __int64,FILE*,const char*,_locale_t,va_list);
_ACRTIMP int __cdecl __stdio_common_vsscanf(unsigned __int64,char const*,size_t,const char*,_locale_t,va_list);

#endif /* _UCRT */

#if defined(_UCRT) && !defined(_NO_CRT_STDIO_INLINE)

static inline int __cdecl vsnprintf(char *buffer, size_t size, const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(3, 0);
static inline int __cdecl vsnprintf(char *buffer, size_t size, const char *format, va_list args)
{
    int ret = __stdio_common_vsprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
                                      buffer, size, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl _vsnprintf(char *buffer, size_t size, const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(3, 0);
static inline int __cdecl _vsnprintf(char *buffer, size_t size, const char *format, va_list args)
{
    int ret = __stdio_common_vsprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION,
                                      buffer, size, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl _vsnprintf_s(char *buffer, size_t size, size_t count, const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(4, 0);
static inline int __cdecl _vsnprintf_s(char *buffer, size_t size, size_t count, const char *format, va_list args)
{
    int ret = __stdio_common_vsnprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, count, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl _snprintf_s(char *buffer, size_t size, size_t count, const char *format, ...) __WINE_CRT_PRINTF_ATTR(4, 5);
static inline int __cdecl _snprintf_s(char *buffer, size_t size, size_t count, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vsnprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, count, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl _vscprintf(const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(1, 0);
static inline int __cdecl _vscprintf(const char *format, va_list args)
{
    int ret = __stdio_common_vsprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
                                      NULL, 0, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl _scprintf(const char *format, ...) __WINE_CRT_PRINTF_ATTR(1, 2);
static inline int __cdecl _scprintf(const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vsprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
                                  NULL, 0, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl vsprintf(char *buffer, const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(2, 0);
static inline int __cdecl vsprintf(char *buffer, const char *format, va_list args)
{
    int ret = __stdio_common_vsprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION,
                                      buffer, -1, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl vsprintf_s(char *buffer, size_t size, const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(3, 0);
static inline int __cdecl vsprintf_s(char *buffer, size_t size, const char *format, va_list args)
{
    int ret = __stdio_common_vsprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, NULL, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl sprintf_s(char *buffer, size_t size, const char *format, ...) __WINE_CRT_PRINTF_ATTR(3, 4);
static inline int __cdecl sprintf_s(char *buffer, size_t size, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vsprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl _vsprintf_p_l(char *buffer, size_t size, const char *format, _locale_t locale, va_list args) __WINE_CRT_PRINTF_ATTR(3, 0);
static inline int __cdecl _vsprintf_p_l(char *buffer, size_t size, const char *format, _locale_t locale, va_list args)
{
    int ret = __stdio_common_vsprintf_p(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, buffer, size, format, locale, args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl vfprintf(FILE *file, const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(2, 0);
static inline int __cdecl vfprintf(FILE *file, const char *format, va_list args)
{
    return __stdio_common_vfprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, file, format, NULL, args);
}

static inline int __cdecl fprintf(FILE *file, const char *format, ...) __WINE_CRT_PRINTF_ATTR(2, 3);
static inline int __cdecl fprintf(FILE *file, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, file, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl vfprintf_s(FILE *file, const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(2, 0);
static inline int __cdecl vfprintf_s(FILE *file, const char *format, va_list args)
{
    return __stdio_common_vfprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, file, format, NULL, args);
}

static inline int __cdecl fprintf_s(FILE *file, const char *format, ...) __WINE_CRT_PRINTF_ATTR(2, 3);
static inline int __cdecl fprintf_s(FILE *file, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, file, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int vprintf(const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(1, 0);
static inline int vprintf(const char *format, va_list args)
{
    return __stdio_common_vfprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, stdout, format, NULL, args);
}

static inline int __cdecl printf(const char *format, ...) __WINE_CRT_PRINTF_ATTR(1, 2);
static inline int __cdecl printf(const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, stdout, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int vprintf_s(const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(1, 0);
static inline int vprintf_s(const char *format, va_list args)
{
    return __stdio_common_vfprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, stdout, format, NULL, args);
}

static inline int __cdecl printf_s(const char *format, ...) __WINE_CRT_PRINTF_ATTR(1, 2);
static inline int __cdecl printf_s(const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, stdout, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl _sprintf_l(char *buffer, const char *format, _locale_t locale, ...) __WINE_CRT_PRINTF_ATTR(2, 4);
static inline int __cdecl _sprintf_l(char *buffer, const char *format, _locale_t locale, ...)
{
    int ret;
    va_list args;

    va_start(args, locale);
    ret = __stdio_common_vsprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION,
                                  buffer, -1, format, locale, args);
    va_end(args);
    return ret < 0 ? -1 : ret;
}

static inline int __cdecl sscanf(const char *buffer, const char *format, ...) __WINE_CRT_SCANF_ATTR(2, 3);
static inline int __cdecl sscanf(const char *buffer, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vsscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS, buffer, -1, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl sscanf_s(const char *buffer, const char *format, ...) __WINE_CRT_SCANF_ATTR(2, 3);
static inline int __cdecl sscanf_s(const char *buffer, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vsscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT, buffer, -1, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl _snscanf_l(const char *buffer, size_t size, const char *format, _locale_t locale, ...) __WINE_CRT_SCANF_ATTR(3, 5);
static inline int __cdecl _snscanf_l(const char *buffer, size_t size, const char *format, _locale_t locale, ...)
{
    int ret;
    va_list args;

    va_start(args, locale);
    ret = __stdio_common_vsscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS, buffer, size, format, locale, args);
    va_end(args);
    return ret;
}

static inline int __cdecl _sscanf_l(const char *buffer, const char *format, _locale_t locale, ...) __WINE_CRT_SCANF_ATTR(2, 4);
static inline int __cdecl _sscanf_l(const char *buffer, const char *format, _locale_t locale, ...)
{
    int ret;
    va_list args;

    va_start(args, locale);
    ret = __stdio_common_vsscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS, buffer, -1, format, locale, args);
    va_end(args);
    return ret;
}

static inline int __cdecl fscanf(FILE *file, const char *format, ...) __WINE_CRT_SCANF_ATTR(2, 3);
static inline int __cdecl fscanf(FILE *file, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS, file, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl fscanf_s(FILE *file, const char *format, ...) __WINE_CRT_SCANF_ATTR(2, 3);
static inline int __cdecl fscanf_s(FILE *file, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT, file, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl scanf(const char *format, ...) __WINE_CRT_SCANF_ATTR(1, 2);
static inline int __cdecl scanf(const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS, stdin, format, NULL, args);
    va_end(args);
    return ret;
}

static inline int __cdecl scanf_s(const char *format, ...) __WINE_CRT_SCANF_ATTR(1, 2);
static inline int __cdecl scanf_s(const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = __stdio_common_vfscanf(_CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT, stdin, format, NULL, args);
    va_end(args);
    return ret;
}

#else /* _UCRT && !_NO_CRT_STDIO_INLINE */

_ACRTIMP int __cdecl _scprintf(const char *,...) __WINE_CRT_PRINTF_ATTR(1, 2);
_ACRTIMP int __cdecl _snprintf_s(char*,size_t,size_t,const char*,...) __WINE_CRT_PRINTF_ATTR(4, 5);
_ACRTIMP int __cdecl _vscprintf(const char*,va_list) __WINE_CRT_PRINTF_ATTR(1, 0);
_ACRTIMP int __cdecl _vsnprintf_s(char*,size_t,size_t,const char*,va_list) __WINE_CRT_PRINTF_ATTR(4, 0);
_ACRTIMP int __cdecl _vsprintf_p_l(char*,size_t,const char*,_locale_t,va_list) __WINE_CRT_PRINTF_ATTR(3, 0);
_ACRTIMP int __cdecl fprintf(FILE*,const char*,...) __WINE_CRT_PRINTF_ATTR(2, 3);
_ACRTIMP int __cdecl fprintf_s(FILE*,const char*,...) __WINE_CRT_PRINTF_ATTR(2, 3);
_ACRTIMP int __cdecl printf(const char*,...) __WINE_CRT_PRINTF_ATTR(1, 2);
_ACRTIMP int __cdecl printf_s(const char*,...) __WINE_CRT_PRINTF_ATTR(1, 2);
_ACRTIMP int __cdecl sprintf_s(char*,size_t,const char*,...) __WINE_CRT_PRINTF_ATTR(3, 4);
_ACRTIMP int __cdecl vfprintf(FILE*,const char*,va_list) __WINE_CRT_PRINTF_ATTR(2, 0);
_ACRTIMP int __cdecl vfprintf_s(FILE*,const char*,va_list) __WINE_CRT_PRINTF_ATTR(2, 0);
_ACRTIMP int __cdecl vprintf(const char*,va_list) __WINE_CRT_PRINTF_ATTR(1, 0);
_ACRTIMP int __cdecl vprintf_s(const char*,va_list) __WINE_CRT_PRINTF_ATTR(1, 0);
_ACRTIMP int __cdecl vsprintf(char*,const char*,va_list) __WINE_CRT_PRINTF_ATTR(2, 0);
_ACRTIMP int __cdecl vsprintf_s(char*,size_t,const char*,va_list) __WINE_CRT_PRINTF_ATTR(3, 0);

_ACRTIMP int __cdecl _vsnprintf(char*,size_t,const char*,va_list) __WINE_CRT_PRINTF_ATTR(3, 0);
static inline int vsnprintf(char *buffer, size_t size, const char *format, va_list args) __WINE_CRT_PRINTF_ATTR(3, 0);
static inline int vsnprintf(char *buffer, size_t size, const char *format, va_list args)
{ return _vsnprintf(buffer,size,format,args); }

_ACRTIMP int __cdecl _snscanf_l(const char*,size_t,const char*,_locale_t,...) __WINE_CRT_SCANF_ATTR(3, 5);
_ACRTIMP int __cdecl _sscanf_l(const char *,const char*,_locale_t,...) __WINE_CRT_SCANF_ATTR(2, 4);
_ACRTIMP int __cdecl fscanf(FILE*,const char*,...) __WINE_CRT_SCANF_ATTR(2, 3);
_ACRTIMP int __cdecl fscanf_s(FILE*,const char*,...) __WINE_CRT_SCANF_ATTR(2, 3);
_ACRTIMP int __cdecl scanf(const char*,...) __WINE_CRT_SCANF_ATTR(1, 2);
_ACRTIMP int __cdecl scanf_s(const char*,...) __WINE_CRT_SCANF_ATTR(1, 2);
_ACRTIMP int __cdecl sscanf(const char*,const char*,...) __WINE_CRT_SCANF_ATTR(2, 3);
_ACRTIMP int __cdecl sscanf_s(const char*,const char*,...) __WINE_CRT_SCANF_ATTR(2, 3);

#endif /* _UCRT && !_NO_CRT_STDIO_INLINE */

#endif /* _STDIO_DEFINED */

#ifdef __cplusplus
}
#endif


static inline FILE* fdopen(int fd, const char *mode) { return _fdopen(fd, mode); }
static inline int fgetchar(void) { return _fgetchar(); }
static inline int fileno(FILE* file) { return _fileno(file); }
static inline int fputchar(int c) { return _fputchar(c); }
static inline int pclose(FILE* file) { return _pclose(file); }
static inline FILE* popen(const char* command, const char* mode) { return _popen(command, mode); }
static inline char* tempnam(const char *dir, const char *prefix) { return _tempnam(dir, prefix); }
#ifndef _UNLINK_DEFINED
static inline int unlink(const char* path) { return _unlink(path); }
#define _UNLINK_DEFINED
#endif

#if !defined(_NO_CRT_STDIO_INLINE)

static inline int __cdecl snprintf(char *buffer, size_t size, const char *format, ...) __WINE_CRT_PRINTF_ATTR(3, 4);
static inline int __cdecl snprintf(char *buffer, size_t size, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = vsnprintf(buffer, size, format, args);
    va_end(args);
    return ret;
}

static inline int __cdecl _snprintf(char *buffer, size_t size, const char *format, ...) __WINE_CRT_PRINTF_ATTR(3, 4);
static inline int __cdecl _snprintf(char *buffer, size_t size, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = _vsnprintf(buffer, size, format, args);
    va_end(args);
    return ret;
}

static inline int __cdecl sprintf(char *buffer, const char *format, ...) __WINE_CRT_PRINTF_ATTR(2, 3);
static inline int __cdecl sprintf(char *buffer, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = _vsnprintf(buffer, (size_t)_CRT_INT_MAX, format, args);
    va_end(args);
    return ret;
}

#else /* !_NO_CRT_STDIO_INLINE */

_ACRTIMP int __cdecl snprintf(char*,size_t,const char*,...) __WINE_CRT_PRINTF_ATTR(3, 4);
_ACRTIMP int __cdecl _snprintf(char*,size_t,const char*,...) __WINE_CRT_PRINTF_ATTR(3, 4);
_ACRTIMP int __cdecl sprintf(char*,const char*,...) __WINE_CRT_PRINTF_ATTR(2, 3);
_ACRTIMP int __cdecl _sprintf_l(char*,const char*,_locale_t,...) __WINE_CRT_PRINTF_ATTR(2, 4);

#endif /* !_NO_CRT_STDIO_INLINE */

static inline wint_t fgetwchar(void) { return _fgetwchar(); }
static inline wint_t fputwchar(wint_t wc) { return _fputwchar(wc); }
static inline int getw(FILE* file) { return _getw(file); }
static inline int putw(int val, FILE* file) { return _putw(val, file); }
static inline FILE* wpopen(const wchar_t* command,const wchar_t* mode) { return _wpopen(command, mode); }

#endif /* __WINE_STDIO_H */
