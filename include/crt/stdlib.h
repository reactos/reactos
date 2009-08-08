/*
 * stdlib.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Definitions for common types, variables, and functions.
 *
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_

/* All the headers include this file. */
#include <_mingw.h>

#define __need_size_t
#define __need_wchar_t
#define __need_NULL
#ifndef RC_INVOKED
#include <stddef.h>
#endif /* RC_INVOKED */

/*
 * RAND_MAX is the maximum value that may be returned by rand.
 * The minimum is zero.
 */
#define	RAND_MAX	0x7FFF

/*
 * These values may be used as exit status codes.
 */
#define	EXIT_SUCCESS	0
#define	EXIT_FAILURE	1

/*
 * Definitions for path name functions.
 * NOTE: All of these values have simply been chosen to be conservatively high.
 *       Remember that with long file names we can no longer depend on
 *       extensions being short.
 */
#ifndef __STRICT_ANSI__

#ifndef MAX_PATH
#define	MAX_PATH	(260)
#endif

#define	_MAX_PATH	MAX_PATH
#define	_MAX_DRIVE	(3)
#define	_MAX_DIR	256
#define	_MAX_FNAME	256
#define	_MAX_EXT	256

#endif	/* Not __STRICT_ANSI__ */


#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif

#if !defined (__STRICT_ANSI__)

/*
 * This seems like a convenient place to declare these variables, which
 * give programs using WinMain (or main for that matter) access to main-ish
 * argc and argv. environ is a pointer to a table of environment variables.
 * NOTE: Strings in _argv and environ are ANSI strings.
 */
extern int	_argc;
extern char**	_argv;

/* imports from runtime dll of the above variables */
#ifdef __MSVCRT__

extern int*  __cdecl __MINGW_NOTHROW   __p___argc(void);
extern char*** __cdecl __MINGW_NOTHROW  __p___argv(void);
extern wchar_t***  __cdecl __MINGW_NOTHROW __p___wargv(void);

#define __argc (*__p___argc())
#define __argv (*__p___argv())
#define __wargv (*__p___wargv())

#else /* !MSVCRT */

#ifndef __DECLSPEC_SUPPORTED

extern int*    _imp____argc_dll;
extern char***  _imp____argv_dll;
#define __argc (*_imp____argc_dll)
#define __argv (*_imp____argv_dll)

#else /* __DECLSPEC_SUPPORTED */

__MINGW_IMPORT int    __argc_dll;
__MINGW_IMPORT char**  __argv_dll;
#define __argc __argc_dll
#define __argv __argv_dll

#endif /* __DECLSPEC_SUPPORTED */

#endif /* __MSVCRT */
#endif /* __STRICT_ANSI__ */
/*
 * Also defined in ctype.h.
 */
#ifndef MB_CUR_MAX
#ifdef __DECLSPEC_SUPPORTED
# ifdef __MSVCRT__
#  define MB_CUR_MAX __mb_cur_max
   __MINGW_IMPORT int __mb_cur_max;
# else		/* not __MSVCRT */
#  define MB_CUR_MAX __mb_cur_max_dll
   __MINGW_IMPORT int __mb_cur_max_dll;
# endif		/* not __MSVCRT */

#else		/* ! __DECLSPEC_SUPPORTED */
# ifdef __MSVCRT__
   extern int* _imp____mbcur_max;
#  define MB_CUR_MAX (*_imp____mb_cur_max)
# else		/* not __MSVCRT */
   extern int*  _imp____mbcur_max_dll;
#  define MB_CUR_MAX (*_imp____mb_cur_max_dll)
# endif 	/* not __MSVCRT */
#endif  	/*  __DECLSPEC_SUPPORTED */
#endif  /* MB_CUR_MAX */

/* 
 * MS likes to declare errno in stdlib.h as well. 
 */

#ifdef _UWIN
#undef errno
extern int errno;
#else
 _CRTIMP int* __cdecl __MINGW_NOTHROW	_errno(void);
#define	errno		(*_errno())
#endif
 _CRTIMP int* __cdecl __MINGW_NOTHROW	__doserrno(void);
#define	_doserrno	(*__doserrno())

#if !defined (__STRICT_ANSI__)
/*
 * Use environ from the DLL, not as a global. 
 */

#ifdef __MSVCRT__
  extern _CRTIMP char *** __cdecl __MINGW_NOTHROW __p__environ(void);
  extern _CRTIMP wchar_t *** __cdecl __MINGW_NOTHROW  __p__wenviron(void);
# define _environ (*__p__environ())
# define _wenviron (*__p__wenviron())
#else /* ! __MSVCRT__ */
# ifndef __DECLSPEC_SUPPORTED
    extern char *** _imp___environ_dll;
#   define _environ (*_imp___environ_dll)
# else /* __DECLSPEC_SUPPORTED */
    __MINGW_IMPORT char ** _environ_dll;
#   define _environ _environ_dll
# endif /* __DECLSPEC_SUPPORTED */
#endif /* ! __MSVCRT__ */

#define environ _environ

#ifdef	__MSVCRT__
/* One of the MSVCRTxx libraries */

#ifndef __DECLSPEC_SUPPORTED
  extern int*	_imp___sys_nerr;
# define	sys_nerr	(*_imp___sys_nerr)
#else /* __DECLSPEC_SUPPORTED */
  __MINGW_IMPORT int	_sys_nerr;
# ifndef _UWIN
#   define	sys_nerr	_sys_nerr
# endif /* _UWIN */
#endif /* __DECLSPEC_SUPPORTED */

#else /* ! __MSVCRT__ */

/* CRTDLL run time library */

#ifndef __DECLSPEC_SUPPORTED
  extern int*	_imp___sys_nerr_dll;
# define sys_nerr	(*_imp___sys_nerr_dll)
#else /* __DECLSPEC_SUPPORTED */
  __MINGW_IMPORT int	_sys_nerr_dll;
# define sys_nerr	_sys_nerr_dll
#endif /* __DECLSPEC_SUPPORTED */

#endif /* ! __MSVCRT__ */

#ifndef __DECLSPEC_SUPPORTED
extern char***	_imp__sys_errlist;
#define	sys_errlist	(*_imp___sys_errlist)
#else /* __DECLSPEC_SUPPORTED */
__MINGW_IMPORT char*	_sys_errlist[];
#ifndef _UWIN
#define	sys_errlist	_sys_errlist
#endif /* _UWIN */
#endif /* __DECLSPEC_SUPPORTED */

/*
 * OS version and such constants.
 */

#ifdef	__MSVCRT__
/* msvcrtxx.dll */

extern _CRTIMP unsigned __cdecl __MINGW_NOTHROW int*	__p__osver(void);
extern _CRTIMP unsigned __cdecl __MINGW_NOTHROW int*	__p__winver(void);
extern _CRTIMP unsigned __cdecl __MINGW_NOTHROW int*	__p__winmajor(void);
extern _CRTIMP unsigned __cdecl __MINGW_NOTHROW int*	__p__winminor(void);

#ifndef __DECLSPEC_SUPPORTED
# define _osver		(*__p__osver())
# define _winver	(*__p__winver())
# define _winmajor	(*__p__winmajor())
# define _winminor	(*__p__winminor())
#else
__MINGW_IMPORT unsigned int _osver;
__MINGW_IMPORT unsigned int _winver;
__MINGW_IMPORT unsigned int _winmajor;
__MINGW_IMPORT unsigned int _winminor;
#endif /* __DECLSPEC_SUPPORTED */

#else
/* Not msvcrtxx.dll, thus crtdll.dll */

#ifndef __DECLSPEC_SUPPORTED

extern unsigned int*	_imp___osver_dll;
extern unsigned int*	_imp___winver_dll;
extern unsigned int*	_imp___winmajor_dll;
extern unsigned int*	_imp___winminor_dll;

#define _osver		(*_imp___osver_dll)
#define _winver		(*_imp___winver_dll)
#define _winmajor	(*_imp___winmajor_dll)
#define _winminor	(*_imp___winminor_dll)

#else /* __DECLSPEC_SUPPORTED */

__MINGW_IMPORT unsigned int	_osver_dll;
__MINGW_IMPORT unsigned int	_winver_dll;
__MINGW_IMPORT unsigned int	_winmajor_dll;
__MINGW_IMPORT unsigned int	_winminor_dll;

#define _osver		_osver_dll
#define _winver		_winver_dll
#define _winmajor	_winmajor_dll
#define _winminor	_winminor_dll

#endif /* __DECLSPEC_SUPPORTED */

#endif

#if defined  __MSVCRT__
/* although the _pgmptr is exported as DATA,
 * be safe and use the access function __p__pgmptr() to get it. */
_CRTIMP char** __cdecl __MINGW_NOTHROW __p__pgmptr(void);
#define _pgmptr     (*__p__pgmptr())
_CRTIMP wchar_t** __cdecl __MINGW_NOTHROW __p__wpgmptr(void);
#define _wpgmptr    (*__p__wpgmptr())
#else /* ! __MSVCRT__ */
# ifndef __DECLSPEC_SUPPORTED
  extern char** __imp__pgmptr_dll;
# define _pgmptr (*_imp___pgmptr_dll)
# else /* __DECLSPEC_SUPPORTED */
 __MINGW_IMPORT char* _pgmptr_dll;
# define _pgmptr _pgmptr_dll
# endif /* __DECLSPEC_SUPPORTED */
/* no wide version in CRTDLL */
#endif /* __MSVCRT__ */

/*
 * This variable determines the default file mode.
 * TODO: Which flags work?
 */
#if !defined (__DECLSPEC_SUPPORTED) || defined (__IN_MINGW_RUNTIME)

#ifdef __MSVCRT__
extern int* _imp___fmode;
#define	_fmode	(*_imp___fmode)
#else
/* CRTDLL */
extern int* _imp___fmode_dll;
#define	_fmode	(*_imp___fmode_dll)
#endif

#else /* __DECLSPEC_SUPPORTED */

#ifdef __MSVCRT__
__MINGW_IMPORT  int _fmode;
#else /* ! __MSVCRT__ */
__MINGW_IMPORT  int _fmode_dll;
#define	_fmode	_fmode_dll
#endif /* ! __MSVCRT__ */

#endif /* __DECLSPEC_SUPPORTED */

#endif /* Not __STRICT_ANSI__ */

_CRTIMP double __cdecl __MINGW_NOTHROW	atof	(const char*);
_CRTIMP int __cdecl __MINGW_NOTHROW	atoi	(const char*);
_CRTIMP long __cdecl __MINGW_NOTHROW 	atol	(const char*);
#if !defined (__STRICT_ANSI__)
_CRTIMP int __cdecl __MINGW_NOTHROW	_wtoi (const wchar_t *);
_CRTIMP long __cdecl __MINGW_NOTHROW _wtol (const wchar_t *);
#endif
_CRTIMP double __cdecl __MINGW_NOTHROW	strtod	(const char*, char**);
#if !defined __NO_ISOCEXT  /*  in libmingwex.a */
float __cdecl __MINGW_NOTHROW strtof (const char * __restrict__, char ** __restrict__);
long double __cdecl __MINGW_NOTHROW strtold (const char * __restrict__, char ** __restrict__);
#endif /* __NO_ISOCEXT */

_CRTIMP long __cdecl __MINGW_NOTHROW	strtol	(const char*, char**, int);
_CRTIMP unsigned long __cdecl __MINGW_NOTHROW	strtoul	(const char*, char**, int);

#ifndef _WSTDLIB_DEFINED
/*  also declared in wchar.h */
_CRTIMP long __cdecl __MINGW_NOTHROW	wcstol	(const wchar_t*, wchar_t**, int);
_CRTIMP unsigned long __cdecl __MINGW_NOTHROW	wcstoul (const wchar_t*, wchar_t**, int);
_CRTIMP double __cdecl __MINGW_NOTHROW	wcstod	(const wchar_t*, wchar_t**);
#if !defined __NO_ISOCEXT /*  in libmingwex.a */
float __cdecl __MINGW_NOTHROW wcstof( const wchar_t * __restrict__, wchar_t ** __restrict__);
long double __cdecl __MINGW_NOTHROW wcstold (const wchar_t * __restrict__, wchar_t ** __restrict__);
#endif /* __NO_ISOCEXT */
#ifdef __MSVCRT__ 
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW _wgetenv(const wchar_t*);
_CRTIMP int __cdecl __MINGW_NOTHROW	_wputenv(const wchar_t*);
_CRTIMP void __cdecl __MINGW_NOTHROW	_wsearchenv(const wchar_t*, const wchar_t*, wchar_t*);
_CRTIMP int __cdecl __MINGW_NOTHROW   	_wsystem(const wchar_t*);
_CRTIMP void __cdecl __MINGW_NOTHROW    _wmakepath(wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*);
_CRTIMP void __cdecl __MINGW_NOTHROW	_wsplitpath (const wchar_t*, wchar_t*, wchar_t*, wchar_t*, wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW   _wfullpath (wchar_t*, const wchar_t*, size_t);
#endif
#define _WSTDLIB_DEFINED
#endif

_CRTIMP size_t __cdecl __MINGW_NOTHROW	wcstombs	(char*, const wchar_t*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW	wctomb		(char*, wchar_t);

_CRTIMP int __cdecl __MINGW_NOTHROW	mblen		(const char*, size_t);
_CRTIMP size_t __cdecl __MINGW_NOTHROW	mbstowcs	(wchar_t*, const char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW	mbtowc		(wchar_t*, const char*, size_t);

_CRTIMP int __cdecl __MINGW_NOTHROW	rand	(void);
_CRTIMP void __cdecl __MINGW_NOTHROW	srand	(unsigned int);

_CRTIMP void* __cdecl __MINGW_NOTHROW	calloc	(size_t, size_t) __MINGW_ATTRIB_MALLOC;
_CRTIMP void* __cdecl __MINGW_NOTHROW	malloc	(size_t) __MINGW_ATTRIB_MALLOC;
_CRTIMP void* __cdecl __MINGW_NOTHROW	realloc	(void*, size_t);
_CRTIMP void __cdecl __MINGW_NOTHROW	free	(void*);
_CRTIMP void __cdecl __MINGW_NOTHROW	abort	(void) __MINGW_ATTRIB_NORETURN;
_CRTIMP void __cdecl __MINGW_NOTHROW	exit	(int) __MINGW_ATTRIB_NORETURN;

/* Note: This is in startup code, not imported directly from dll */
int __cdecl __MINGW_NOTHROW	atexit	(void (*)(void));

_CRTIMP int __cdecl __MINGW_NOTHROW	system	(const char*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	getenv	(const char*);

/* bsearch and qsort are also in non-ANSI header search.h  */
_CRTIMP void* __cdecl bsearch (const void*, const void*, size_t, size_t, 
			       int (*)(const void*, const void*));
_CRTIMP void __cdecl qsort(void*, size_t, size_t,
			   int (*)(const void*, const void*));

_CRTIMP int __cdecl __MINGW_NOTHROW	abs	(int) __MINGW_ATTRIB_CONST;
_CRTIMP long __cdecl __MINGW_NOTHROW	labs	(long) __MINGW_ATTRIB_CONST;

/*
 * div_t and ldiv_t are structures used to return the results of div and
 * ldiv.
 *
 * NOTE: div and ldiv appear not to work correctly unless
 *       -fno-pcc-struct-return is specified. This is included in the
 *       mingw32 specs file.
 */
typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;

_CRTIMP div_t __cdecl __MINGW_NOTHROW	div	(int, int) __MINGW_ATTRIB_CONST;
_CRTIMP ldiv_t __cdecl __MINGW_NOTHROW	ldiv	(long, long) __MINGW_ATTRIB_CONST;

#if !defined (__STRICT_ANSI__)

/*
 * NOTE: Officially the three following functions are obsolete. The Win32 API
 *       functions SetErrorMode, Beep and Sleep are their replacements.
 */
_CRTIMP void __cdecl __MINGW_NOTHROW	_beep (unsigned int, unsigned int) __MINGW_ATTRIB_DEPRECATED;
/* Not to be confused with  _set_error_mode (int).  */
_CRTIMP void __cdecl __MINGW_NOTHROW	_seterrormode (int) __MINGW_ATTRIB_DEPRECATED;
_CRTIMP void __cdecl __MINGW_NOTHROW	_sleep (unsigned long) __MINGW_ATTRIB_DEPRECATED;

_CRTIMP void __cdecl __MINGW_NOTHROW	_exit	(int) __MINGW_ATTRIB_NORETURN;

/* _onexit is MS extension. Use atexit for portability.  */
/* Note: This is in startup code, not imported directly from dll */
typedef  int (* _onexit_t)(void);
_onexit_t __cdecl __MINGW_NOTHROW _onexit( _onexit_t );

_CRTIMP int __cdecl __MINGW_NOTHROW	_putenv	(const char*);
_CRTIMP void __cdecl __MINGW_NOTHROW	_searchenv (const char*, const char*, char*);


_CRTIMP char* __cdecl __MINGW_NOTHROW	_ecvt (double, int, int*, int*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_fcvt (double, int, int*, int*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_gcvt (double, int, char*);

_CRTIMP void __cdecl __MINGW_NOTHROW	_makepath (char*, const char*, const char*, const char*, const char*);
_CRTIMP void __cdecl __MINGW_NOTHROW	_splitpath (const char*, char*, char*, char*, char*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_fullpath (char*, const char*, size_t);

_CRTIMP char* __cdecl __MINGW_NOTHROW	_itoa (int, char*, int);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_ltoa (long, char*, int);
_CRTIMP char* __cdecl __MINGW_NOTHROW   _ultoa(unsigned long, char*, int);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW  _itow (int, wchar_t*, int);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW  _ltow (long, wchar_t*, int);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW  _ultow (unsigned long, wchar_t*, int);

_CRTIMP __int64 __cdecl __MINGW_NOTHROW	_atoi64(const char *);
    
#ifdef __MSVCRT__
_CRTIMP char* __cdecl __MINGW_NOTHROW	_i64toa(__int64, char *, int);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_ui64toa(unsigned __int64, char *, int);
_CRTIMP __int64 __cdecl __MINGW_NOTHROW	_wtoi64(const wchar_t *);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW _i64tow(__int64, wchar_t *, int);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW _ui64tow(unsigned __int64, wchar_t *, int);


_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _rotl(unsigned int, int) __MINGW_ATTRIB_CONST;
_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _rotr(unsigned int, int) __MINGW_ATTRIB_CONST;
_CRTIMP unsigned long __cdecl __MINGW_NOTHROW _lrotl(unsigned long, int) __MINGW_ATTRIB_CONST;
_CRTIMP unsigned long __cdecl __MINGW_NOTHROW _lrotr(unsigned long, int) __MINGW_ATTRIB_CONST;

_CRTIMP int __cdecl __MINGW_NOTHROW _set_error_mode (int);
#define _OUT_TO_DEFAULT	0
#define _OUT_TO_STDERR	1
#define _OUT_TO_MSGBOX	2
#define _REPORT_ERRMODE	3

#if __MSVCRT_VERSION__ >= 0x800
#ifndef _INTPTR_T_DEFINED
#define _INTPTR_T_DEFINED
#ifdef _WIN64
  typedef __int64 intptr_t;
#else
  typedef int intptr_t;
#endif
#endif
_CRTIMP unsigned int __cdecl __MINGW_NOTHROW _set_abort_behavior (unsigned int, unsigned int);
/* These masks work with msvcr80.dll version 8.0.50215.44 (a beta release).  */
#define _WRITE_ABORT_MSG 1
#define _CALL_REPORTFAULT 2

typedef void (* _invalid_parameter_handler) (const wchar_t *,
					     const wchar_t *,
					     const wchar_t *,
					     unsigned int,
					     uintptr_t);
_invalid_parameter_handler _set_invalid_parameter_handler (_invalid_parameter_handler);
#endif /* __MSVCRT_VERSION__ >= 0x800 */
#endif /* __MSVCRT__ */

#ifndef	_NO_OLDNAMES

_CRTIMP int __cdecl __MINGW_NOTHROW	putenv (const char*);
_CRTIMP void __cdecl __MINGW_NOTHROW	searchenv (const char*, const char*, char*);

_CRTIMP char* __cdecl __MINGW_NOTHROW	itoa (int, char*, int);
_CRTIMP char* __cdecl __MINGW_NOTHROW	ltoa (long, char*, int);

#ifndef _UWIN
_CRTIMP char* __cdecl __MINGW_NOTHROW	ecvt (double, int, int*, int*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	fcvt (double, int, int*, int*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	gcvt (double, int, char*);
#endif /* _UWIN */
#endif	/* Not _NO_OLDNAMES */

#endif	/* Not __STRICT_ANSI__ */

/* C99 names */

#if !defined __NO_ISOCEXT /* externs in static libmingwex.a */

/* C99 name for _exit */
void __cdecl __MINGW_NOTHROW _Exit(int) __MINGW_ATTRIB_NORETURN;
#ifndef __STRICT_ANSI__   /* inline using non-ansi functions */
__CRT_INLINE void __cdecl __MINGW_NOTHROW _Exit(int __status)
	{  _exit (__status); }
#endif 

typedef struct { long long quot, rem; } lldiv_t;

lldiv_t	__cdecl __MINGW_NOTHROW lldiv (long long, long long) __MINGW_ATTRIB_CONST;

long long __cdecl __MINGW_NOTHROW llabs(long long);
__CRT_INLINE long long __cdecl __MINGW_NOTHROW llabs(long long _j)
  {return (_j >= 0 ? _j : -_j);}

long long  __cdecl __MINGW_NOTHROW strtoll (const char* __restrict__, char** __restrict, int);
unsigned long long  __cdecl __MINGW_NOTHROW strtoull (const char* __restrict__, char** __restrict__, int);

#if defined (__MSVCRT__) /* these are stubs for MS _i64 versions */ 
long long  __cdecl __MINGW_NOTHROW atoll (const char *);

#if !defined (__STRICT_ANSI__)
long long  __cdecl __MINGW_NOTHROW wtoll (const wchar_t *);
char* __cdecl __MINGW_NOTHROW lltoa (long long, char *, int);
char* __cdecl __MINGW_NOTHROW ulltoa (unsigned long long , char *, int);
wchar_t* __cdecl __MINGW_NOTHROW lltow (long long, wchar_t *, int);
wchar_t* __cdecl __MINGW_NOTHROW ulltow (unsigned long long, wchar_t *, int);

  /* inline using non-ansi functions */
__CRT_INLINE long long  __cdecl __MINGW_NOTHROW atoll (const char * _c)
	{ return _atoi64 (_c); }
__CRT_INLINE char*  __cdecl __MINGW_NOTHROW lltoa (long long _n, char * _c, int _i)
	{ return _i64toa (_n, _c, _i); }
__CRT_INLINE char*  __cdecl __MINGW_NOTHROW ulltoa (unsigned long long _n, char * _c, int _i)
	{ return _ui64toa (_n, _c, _i); }
__CRT_INLINE long long  __cdecl __MINGW_NOTHROW wtoll (const wchar_t * _w)
 	{ return _wtoi64 (_w); }
__CRT_INLINE wchar_t*  __cdecl __MINGW_NOTHROW lltow (long long _n, wchar_t * _w, int _i)
	{ return _i64tow (_n, _w, _i); } 
__CRT_INLINE wchar_t*  __cdecl __MINGW_NOTHROW ulltow (unsigned long long _n, wchar_t * _w, int _i)
	{ return _ui64tow (_n, _w, _i); } 
#endif /* (__STRICT_ANSI__)  */

#endif /* __MSVCRT__ */

#endif /* !__NO_ISOCEXT */


#ifdef __cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _STDLIB_H_ */


