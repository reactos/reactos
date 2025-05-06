/*
 * Standard library definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STDLIB_H
#define __WINE_STDLIB_H

#include <corecrt_malloc.h>
#include <corecrt_wstdlib.h>
#include <limits.h>

#include <pshpack8.h>

typedef struct
{
    float f;
} _CRT_FLOAT;

typedef struct
{
    double x;
} _CRT_DOUBLE;

typedef struct
{
    unsigned char ld[10];
} _LDOUBLE;

#define EXIT_SUCCESS        0
#define EXIT_FAILURE        -1
#define RAND_MAX            0x7FFF

#ifndef _MAX_PATH
#define _MAX_DRIVE          3
#define _MAX_FNAME          256
#define _MAX_DIR            _MAX_FNAME
#define _MAX_EXT            _MAX_FNAME
#define _MAX_PATH           260
#endif

/* Make the secure string functions (names end in "_s") truncate their output */
#define _TRUNCATE  ((size_t)-1)

typedef struct _div_t {
    int quot;
    int rem;
} div_t;

typedef struct _ldiv_t {
    __msvcrt_long quot;
    __msvcrt_long rem;
} ldiv_t;

typedef struct _lldiv_t {
    __int64 quot;
    __int64 rem;
} lldiv_t;


#define _countof(x) (sizeof(x)/sizeof((x)[0]))

#define __max(a,b) (((a) > (b)) ? (a) : (b))
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#ifndef __cplusplus
#define max(a,b)   (((a) > (b)) ? (a) : (b))
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#endif

/* _set_error_mode() constants */
#define _OUT_TO_DEFAULT      0
#define _OUT_TO_STDERR       1
#define _OUT_TO_MSGBOX       2
#define _REPORT_ERRMODE      3

/* _set_abort_behavior codes */
#define _WRITE_ABORT_MSG     1
#define _CALL_REPORTFAULT    2

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__i386__) || defined(_UCRT)

_ACRTIMP unsigned int* __cdecl __p__osver(void);
_ACRTIMP unsigned int* __cdecl __p__winver(void);
_ACRTIMP unsigned int* __cdecl __p__winmajor(void);
_ACRTIMP unsigned int* __cdecl __p__winminor(void);
_ACRTIMP int*          __cdecl __p___argc(void);
_ACRTIMP char***       __cdecl __p___argv(void);
_ACRTIMP wchar_t***    __cdecl __p___wargv(void);
_ACRTIMP char***       __cdecl __p__environ(void);
_ACRTIMP wchar_t***    __cdecl __p__wenviron(void);
_ACRTIMP int*          __cdecl __p__fmode(void);
#define _osver               (*__p__osver())
#define _winver              (*__p__winver())
#define _winmajor            (*__p__winmajor())
#define _winminor            (*__p__winminor())
#define __argc               (*__p___argc())
#define __argv               (*__p___argv())
#define __wargv              (*__p___wargv())
#define _environ             (*__p__environ())
#define _wenviron            (*__p__wenviron())
#define _fmode               (*__p__fmode())

#else  /* __i386__ */

extern unsigned int _osver;
extern unsigned int _winver;
extern unsigned int _winmajor;
extern unsigned int _winminor;
extern int __argc;
extern char **__argv;
extern wchar_t **__wargv;
extern char **_environ;
extern wchar_t **_wenviron;
extern unsigned int _fmode;

#endif  /* __i386__ */

_ACRTIMP int             __cdecl ___mb_cur_max_func(void);
_ACRTIMP int             __cdecl ___mb_cur_max_l_func(_locale_t);
#define __mb_cur_max             ___mb_cur_max_func()
#define MB_CUR_MAX               ___mb_cur_max_func()
_ACRTIMP __msvcrt_ulong* __cdecl __doserrno(void);
#define _doserrno              (*__doserrno())
_ACRTIMP int*            __cdecl _errno(void);
#define errno                  (*_errno())
_ACRTIMP int*            __cdecl __sys_nerr(void);
#define _sys_nerr              (*__sys_nerr())

/* FIXME: We need functions to access these:
 * int _sys_nerr;
 * char** _sys_errlist;
 */

_ACRTIMP errno_t       __cdecl _get_doserrno(int*);
_ACRTIMP errno_t       __cdecl _get_errno(int*);
_ACRTIMP errno_t       __cdecl _set_doserrno(int);
_ACRTIMP errno_t       __cdecl _set_errno(int);

_ACRTIMP errno_t       __cdecl _set_fmode(int);
_ACRTIMP errno_t       __cdecl _get_fmode(int*);

#ifndef _CRT_ONEXIT_T_DEFINED
#define _CRT_ONEXIT_T_DEFINED
typedef int (__cdecl *_onexit_t)(void);
#endif


_ACRTIMP int           __cdecl _atodbl(_CRT_DOUBLE*,char*);
_ACRTIMP int           __cdecl _atodbl_l(_CRT_DOUBLE*,char*,_locale_t);
_ACRTIMP int           __cdecl _atoflt(_CRT_FLOAT*,char*);
_ACRTIMP int           __cdecl _atoflt_l(_CRT_FLOAT*,char*,_locale_t);
_ACRTIMP __int64       __cdecl _atoi64(const char*);
_ACRTIMP int           __cdecl _atoldbl(_LDOUBLE*,char*);
_ACRTIMP void          __cdecl _beep(unsigned int,unsigned int);
_ACRTIMP unsigned short   __cdecl _byteswap_ushort(unsigned short);
_ACRTIMP __msvcrt_ulong   __cdecl _byteswap_ulong(__msvcrt_ulong);
_ACRTIMP unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64);
_ACRTIMP char*         __cdecl _ecvt(double,int,int*,int*);
_ACRTIMP char*         __cdecl _fcvt(double,int,int*,int*);
_ACRTIMP char*         __cdecl _fullpath(char*,const char*,size_t);
_ACRTIMP char*         __cdecl _gcvt(double,int,char*);
_ACRTIMP errno_t       __cdecl _gcvt_s(char*, size_t, double, int);
_ACRTIMP char*         __cdecl _i64toa(__int64,char*,int);
_ACRTIMP errno_t       __cdecl _i64toa_s(__int64, char*, size_t, int);
_ACRTIMP char*         __cdecl _itoa(int,char*,int);
_ACRTIMP errno_t       __cdecl _itoa_s(int,char*,size_t,int);
_ACRTIMP char*         __cdecl _ltoa(__msvcrt_long,char*,int);
_ACRTIMP errno_t       __cdecl _ltoa_s(__msvcrt_long, char*, size_t, int);
#ifndef _lrotl
_ACRTIMP __msvcrt_ulong __cdecl _lrotl(__msvcrt_ulong,int);
#endif
#ifndef _lrotr
_ACRTIMP __msvcrt_ulong __cdecl _lrotr(__msvcrt_ulong,int);
#endif
_ACRTIMP void          __cdecl _makepath(char*,const char*,const char*,const char*,const char*);
_ACRTIMP int           __cdecl _makepath_s(char*,size_t,const char*,const char*,const char*,const char*);
_ACRTIMP size_t        __cdecl _mbstrlen(const char*);
_ACRTIMP _onexit_t     __cdecl _onexit(_onexit_t);
_ACRTIMP int           __cdecl _putenv(const char*);
_ACRTIMP errno_t       __cdecl _putenv_s(const char*,const char*);
#ifndef _rotl
_ACRTIMP unsigned int  __cdecl _rotl(unsigned int,int);
#endif
#ifndef _rotr
_ACRTIMP unsigned int  __cdecl _rotr(unsigned int,int);
#endif
_ACRTIMP void          __cdecl _searchenv(const char*,const char*,char*);
_ACRTIMP int           __cdecl _set_error_mode(int);
_ACRTIMP void          __cdecl _seterrormode(int);
_ACRTIMP void          __cdecl _sleep(__msvcrt_ulong);
_ACRTIMP void          __cdecl _splitpath(const char*,char*,char*,char*,char*);
_ACRTIMP errno_t       __cdecl _splitpath_s(const char*,char*,size_t,char*,size_t,char*,size_t,char*,size_t);
_ACRTIMP void          __cdecl _swab(char*,char*,int);
_ACRTIMP char*         __cdecl _ui64toa(unsigned __int64,char*,int);
_ACRTIMP errno_t       __cdecl _ui64toa_s(unsigned __int64,char*,size_t,int);
_ACRTIMP char*         __cdecl _ultoa(__msvcrt_ulong,char*,int);
_ACRTIMP errno_t       __cdecl _ultoa_s(__msvcrt_ulong,char*,size_t,int);

_ACRTIMP DECLSPEC_NORETURN void __cdecl _Exit(int);
_ACRTIMP DECLSPEC_NORETURN void __cdecl _exit(int);
_ACRTIMP DECLSPEC_NORETURN void __cdecl abort(void);
_ACRTIMP int           __cdecl abs(int);
extern int             __cdecl atexit(void (__cdecl *)(void));
_ACRTIMP double        __cdecl atof(const char*);
_ACRTIMP int           __cdecl atoi(const char*);
_ACRTIMP int           __cdecl _atoi_l(const char*,_locale_t);
_ACRTIMP __msvcrt_long __cdecl atol(const char*);
_ACRTIMP __int64       __cdecl atoll(const char*);
#ifndef __i386__
_ACRTIMP div_t  __cdecl div(int,int);
_ACRTIMP ldiv_t __cdecl ldiv(__msvcrt_long,__msvcrt_long);
#endif
_ACRTIMP lldiv_t       __cdecl lldiv(__int64,__int64);
_ACRTIMP DECLSPEC_NORETURN void __cdecl exit(int);
_ACRTIMP char*         __cdecl getenv(const char*);
_ACRTIMP errno_t       __cdecl getenv_s(size_t*,char*,size_t,const char*);
_ACRTIMP __msvcrt_long __cdecl labs(__msvcrt_long);
_ACRTIMP __int64       __cdecl llabs(__int64);
_ACRTIMP int           __cdecl mblen(const char*,size_t);
_ACRTIMP void          __cdecl perror(const char*);
_ACRTIMP int           __cdecl rand(void);
_ACRTIMP errno_t       __cdecl rand_s(unsigned int*);
_ACRTIMP void          __cdecl srand(unsigned int);
_ACRTIMP float         __cdecl strtof(const char*,char**);
_ACRTIMP float         __cdecl _strtof_l(const char*,char**,_locale_t);
_ACRTIMP double        __cdecl strtod(const char*,char**);
_ACRTIMP double        __cdecl _strtod_l(const char*,char**,_locale_t);
#if defined(__GNUC__) || _MSVCR_VER < 120
static inline long double strtold(const char *string, char **endptr) { return strtod(string, endptr); }
static inline long double _strtold_l(const char *string, char **endptr, _locale_t locale) { return _strtod_l(string, endptr, locale); }
#else
_ACRTIMP long double   __cdecl strtold(const char*,char**);
_ACRTIMP long double   __cdecl _strtold_l(const char*,char**,_locale_t);
#endif
_ACRTIMP __msvcrt_long __cdecl strtol(const char*,char**,int);
_ACRTIMP __msvcrt_ulong __cdecl strtoul(const char*,char**,int);
_ACRTIMP __int64       __cdecl _strtoll_l(const char*,char**,int,_locale_t);
_ACRTIMP unsigned __int64 __cdecl _strtoull_l(const char*,char**,int,_locale_t);
_ACRTIMP __int64       __cdecl _strtoi64(const char*,char**,int);
_ACRTIMP __int64       __cdecl _strtoi64_l(const char*,char**,int,_locale_t);
_ACRTIMP unsigned __int64 __cdecl _strtoui64(const char*,char**,int);
_ACRTIMP unsigned __int64 __cdecl _strtoui64_l(const char*,char**,int,_locale_t);
_ACRTIMP int           __cdecl system(const char*);
_ACRTIMP void*         __cdecl bsearch(const void*,const void*,size_t,size_t,int (__cdecl *)(const void*,const void*));
_ACRTIMP void          __cdecl qsort(void*,size_t,size_t,int (__cdecl *)(const void*,const void*));
_ACRTIMP void          __cdecl qsort_s(void*,size_t,size_t,int (__cdecl *)(void*,const void*,const void*),void*);
_ACRTIMP unsigned int  __cdecl _set_abort_behavior(unsigned int flags, unsigned int mask);

typedef void (__cdecl *_purecall_handler)(void);
_ACRTIMP _purecall_handler __cdecl _set_purecall_handler(_purecall_handler);
_ACRTIMP _purecall_handler __cdecl _get_purecall_handler(void);

typedef void (__cdecl *_invalid_parameter_handler)(const wchar_t*, const wchar_t*, const wchar_t*, unsigned, uintptr_t);
_ACRTIMP _invalid_parameter_handler __cdecl _set_invalid_parameter_handler(_invalid_parameter_handler);
_ACRTIMP _invalid_parameter_handler __cdecl _get_invalid_parameter_handler(void);
_ACRTIMP _invalid_parameter_handler __cdecl _get_thread_local_invalid_parameter_handler(void);
_ACRTIMP _invalid_parameter_handler __cdecl _set_thread_local_invalid_parameter_handler(_invalid_parameter_handler);
void __cdecl _invalid_parameter(const wchar_t *expr, const wchar_t *func, const wchar_t *file,
                                unsigned int line, uintptr_t arg);

#ifdef __cplusplus
extern "C++" {

template <size_t size>
inline errno_t getenv_s(size_t *ret, char (&buf)[size], const char *var)
{
    return getenv_s(ret, buf, size, var);
}

inline long abs(long const x) throw() { return labs(x); }
inline long long abs(long long const x) throw() { return llabs(x); }

} /* extern "C++" */

}
#endif


#define environ _environ
#define onexit_t _onexit_t

static inline char* ecvt(double value, int ndigit, int* decpt, int* sign) { return _ecvt(value, ndigit, decpt, sign); }
static inline char* fcvt(double value, int ndigit, int* decpt, int* sign) { return _fcvt(value, ndigit, decpt, sign); }
static inline char* gcvt(double value, int ndigit, char* buf) { return _gcvt(value, ndigit, buf); }
static inline char* itoa(int value, char* str, int radix) { return _itoa(value, str, radix); }
static inline char* ltoa(__msvcrt_long value, char* str, int radix) { return _ltoa(value, str, radix); }
static inline _onexit_t onexit(_onexit_t func) { return _onexit(func); }
static inline int putenv(const char* str) { return _putenv(str); }
static inline __int64 strtoll(const char *ptr, char **endptr, int base) { return _strtoi64(ptr, endptr, base); }
static inline unsigned __int64 __cdecl strtoull(const char *ptr, char **endptr, int base) { return _strtoui64(ptr, endptr, base); }
static inline void swab(char* src, char* dst, int len) { _swab(src, dst, len); }
static inline char* ultoa(__msvcrt_ulong value, char* str, int radix) { return _ultoa(value, str, radix); }

#ifdef __i386__
static inline div_t __wine_msvcrt_div(int num, int denom)
{
    extern unsigned __int64 div(int,int);
    div_t ret;
    unsigned __int64 res = div(num,denom);
    ret.quot = (int)res;
    ret.rem  = (int)(res >> 32);
    return ret;
}
static inline ldiv_t __wine_msvcrt_ldiv(__msvcrt_long num, __msvcrt_long denom)
{
    extern unsigned __int64 ldiv(__msvcrt_long,__msvcrt_long);
    ldiv_t ret;
    unsigned __int64 res = ldiv(num,denom);
    ret.quot = (__msvcrt_long)res;
    ret.rem  = (__msvcrt_long)(res >> 32);
    return ret;
}
#define div(num,denom) __wine_msvcrt_div(num,denom)
#define ldiv(num,denom) __wine_msvcrt_ldiv(num,denom)
#endif

#include <poppack.h>

#endif /* __WINE_STDLIB_H */
