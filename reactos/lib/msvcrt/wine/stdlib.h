/*
 * Standard library definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STDLIB_H
#define __WINE_STDLIB_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void*)0)
#endif
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

#ifndef _MSC_VER
# ifndef __int64
#  define __int64 long long
# endif
#endif

#ifndef USE_MSVCRT_PREFIX
#define EXIT_SUCCESS        0
#define EXIT_FAILURE        -1
#define RAND_MAX            0x7FFF
#else
#define MSVCRT_RAND_MAX     0x7FFF
#endif /* USE_MSVCRT_PREFIX */

#ifndef _MAX_PATH
#define _MAX_DRIVE          3
#define _MAX_FNAME          256
#define _MAX_DIR            _MAX_FNAME
#define _MAX_EXT            _MAX_FNAME
#define _MAX_PATH           260
#endif


typedef struct MSVCRT(_div_t) {
    int quot;
    int rem;
} MSVCRT(div_t);

typedef struct MSVCRT(_ldiv_t) {
    long quot;
    long rem;
} MSVCRT(ldiv_t);

#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

#define __max(a,b) (((a) > (b)) ? (a) : (b))
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#ifndef __cplusplus
#ifndef max
#define max(a,b)   (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#endif
#endif

/* _set_error_mode() constants */
#define _OUT_TO_DEFAULT      0
#define _OUT_TO_STDERR       1
#define _OUT_TO_MSGBOX       2
#define _REPORT_ERRMODE      3


#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int*         __p__osver();
extern unsigned int*         __p__winver();
extern unsigned int*         __p__winmajor();
extern unsigned int*         __p__winminor();
#define _osver             (*__p__osver())
#define _winver            (*__p__winver())
#define _winmajor          (*__p__winmajor())
#define _winminor          (*__p__winminor())

extern int*                  __p___argc(void);
extern char***               __p___argv(void);
extern MSVCRT(wchar_t)***    __p___wargv(void);
extern char***               __p__environ(void);
extern MSVCRT(wchar_t)***    __p__wenviron(void);
extern int*                  __p___mb_cur_max(void);
//extern unsigned long*        __doserrno(void);
extern unsigned int*         __p__fmode(void);
/* FIXME: We need functions to access these:
 * int _sys_nerr;
 * char** _sys_errlist;
 */
#ifndef USE_MSVCRT_PREFIX
#define __argc             (*__p___argc())
#define __argv             (*__p___argv())
#define __wargv            (*__p___wargv())
#define _environ           (*__p__environ())
#define _wenviron          (*__p__wenviron())
#define __mb_cur_max       (*__p___mb_cur_max())
#define _doserrno          (*__doserrno())
#define _fmode             (*_fmode)
#endif /* USE_MSVCRT_PREFIX */


extern int*           MSVCRT(_errno)(void);
#ifndef USE_MSVCRT_PREFIX
# define errno        (*_errno())
#else
# define MSVCRT_errno (*MSVCRT__errno())
#endif


//typedef int (*_onexit_t)(void);


__int64     _atoi64(const char*);
long double _atold(const char*);
void        _beep(unsigned int,unsigned int);
char*       _ecvt(double,int,int*,int*);
char*       _fcvt(double,int,int*,int*);
char*       _fullpath(char*,const char*,MSVCRT(size_t));
char*       _gcvt(double,int,char*);
char*       _i64toa(__int64,char*,int);
char*       _itoa(int,char*,int);
char*       _ltoa(long,char*,int);
unsigned long _lrotl(unsigned long,int);
unsigned long _lrotr(unsigned long,int);
void        _makepath(char*,const char*,const char*,const char*,const char*);
MSVCRT(size_t) _mbstrlen(const char*);
//_onexit_t   _onexit(_onexit_t);
int         _putenv(const char*);
unsigned int _rotl(unsigned int,int);
unsigned int _rotr(unsigned int,int);
void        _searchenv(const char*,const char*,char*);
int         _set_error_mode(int);
void        _seterrormode(int);
void        _sleep(unsigned long);
void        _splitpath(const char*,char*,char*,char*,char*);
long double _strtold(const char*,char**);
//void        _swab(char*,char*,int); //REACTOS
_CRTIMP void __cdecl	_swab (const char*, char*, size_t);
char*       _ui64toa(unsigned __int64,char*,int);
char*       _ultoa(unsigned long,char*,int);

void        MSVCRT(_exit)(int);
void        MSVCRT(abort)();
int         MSVCRT(abs)(int);
int         MSVCRT(atexit)(void (*)(void));
double      MSVCRT(atof)(const char*);
int         MSVCRT(atoi)(const char*);
long        MSVCRT(atol)(const char*);
void*       MSVCRT(calloc)(MSVCRT(size_t),MSVCRT(size_t));
#ifndef __i386__
MSVCRT(div_t) MSVCRT(div)(int,int);
MSVCRT(ldiv_t) MSVCRT(ldiv)(long,long);
#endif
void        MSVCRT(exit)(int);
void        MSVCRT(free)(void*);
char*       MSVCRT(getenv)(const char*);
long        MSVCRT(labs)(long);
void*       MSVCRT(malloc)(MSVCRT(size_t));
int         MSVCRT(mblen)(const char*,MSVCRT(size_t));
void        MSVCRT(perror)(const char*);
int         MSVCRT(rand)(void);
void*       MSVCRT(realloc)(void*,MSVCRT(size_t));
void        MSVCRT(srand)(unsigned int);
double      MSVCRT(strtod)(const char*,char**);
long        MSVCRT(strtol)(const char*,char**,int);
unsigned long MSVCRT(strtoul)(const char*,char**,int);
int         MSVCRT(system)(const char*);
void*       MSVCRT(bsearch)(const void*,const void*,MSVCRT(size_t),MSVCRT(size_t),
                            int (*)(const void*,const void*));
void        MSVCRT(qsort)(void*,MSVCRT(size_t),MSVCRT(size_t),
                          int (*)(const void*,const void*));

#ifndef MSVCRT_WSTDLIB_DEFINED
#define MSVCRT_WSTDLIB_DEFINED
MSVCRT(wchar_t)*_itow(int,MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)*_i64tow(__int64,MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)*_ltow(long,MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)*_ui64tow(unsigned __int64,MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)*_ultow(unsigned long,MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)*_wfullpath(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
MSVCRT(wchar_t)*_wgetenv(const MSVCRT(wchar_t)*);
void            _wmakepath(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
void            _wperror(const MSVCRT(wchar_t)*);
int             _wputenv(const MSVCRT(wchar_t)*);
void            _wsearchenv(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(wchar_t)*);
void            _wsplitpath(const MSVCRT(wchar_t)*,MSVCRT(wchar_t)*,MSVCRT(wchar_t)*,MSVCRT(wchar_t)*,MSVCRT(wchar_t)*);
int             _wsystem(const MSVCRT(wchar_t)*);
int             _wtoi(const MSVCRT(wchar_t)*);
__int64         _wtoi64(const MSVCRT(wchar_t)*);
long            _wtol(const MSVCRT(wchar_t)*);

MSVCRT(size_t) MSVCRT(mbstowcs)(MSVCRT(wchar_t)*,const char*,MSVCRT(size_t));
int            MSVCRT(mbtowc)(MSVCRT(wchar_t)*,const char*,MSVCRT(size_t));
double         MSVCRT(wcstod)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t)**);
long           MSVCRT(wcstol)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t)**,int);
MSVCRT(size_t) MSVCRT(wcstombs)(char*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
unsigned long  MSVCRT(wcstoul)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t)**,int);
int            MSVCRT(wctomb)(char*,MSVCRT(wchar_t));
#endif /* MSVCRT_WSTDLIB_DEFINED */

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define environ _environ
#define onexit_t _onexit_t

static inline char* ecvt(double value, int ndigit, int* decpt, int* sign) { return _ecvt(value, ndigit, decpt, sign); }
static inline char* fcvt(double value, int ndigit, int* decpt, int* sign) { return _fcvt(value, ndigit, decpt, sign); }
static inline char* gcvt(double value, int ndigit, char* buf) { return _gcvt(value, ndigit, buf); }
static inline char* itoa(int value, char* str, int radix) { return _itoa(value, str, radix); }
static inline char* ltoa(long value, char* str, int radix) { return _ltoa(value, str, radix); }
static inline _onexit_t onexit(_onexit_t func) { return _onexit(func); }
static inline int putenv(const char* str) { return _putenv(str); }
static inline void swab(char* src, char* dst, int len) { _swab(src, dst, len); }
static inline char* ultoa(unsigned long value, char* str, int radix) { return _ultoa(value, str, radix); }

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
static inline ldiv_t __wine_msvcrt_ldiv(long num, long denom)
{
    extern unsigned __int64 ldiv(long,long);
    ldiv_t ret;
    unsigned __int64 res = ldiv(num,denom);
    ret.quot = (long)res;
    ret.rem  = (long)(res >> 32);
    return ret;
}
#define div(num,denom) __wine_msvcrt_div(num,denom)
#define ldiv(num,denom) __wine_msvcrt_ldiv(num,denom)
#endif

#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_STDLIB_H */
