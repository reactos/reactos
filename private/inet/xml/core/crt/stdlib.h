#ifdef UNIX
#ifndef __STDLIB_WRAPPER_H__
#define __STDLIB_WRAPPER_H__
 
// #XXXinclude "mwconfig.h"
// #XXXinclude MW_INCLUDE_ANSI_HEADER(stdlib.h)
#include <stddef.h>
#include <sys/param.h>
#include <mwwstdlib.h>

MWBEGIN_EXTERN_C_BLOCK

/* Undef macros defined by SAG to prevent compiler warnings. We do the same
 * thing, but SAG headers are not always included. 
 */
#undef _ltoa
#undef _ultoa
#define __min(a,b)	(((a)<(b))?(a):(b))
#define __max(a,b)	(((a)>(b))?(a):(b))
#define _itoa		itoa
#define ltoa		Mwltoa
#define _ltoa		ltoa
#define ultoa		Mwultoa
#define _ultoa		ultoa
#define _doserrno	errno
#define _gcvt		gcvt
#define _ecvt		ecvt
#define _fcvt		fcvt
#define _putenv		putenv
#define _swab		swab
#define _isatty		isatty
#ifdef MAXNAMELEN
#define _MAX_FNAME	MAXNAMELEN
#else
#define _MAX_FNAME	256
#endif
#define _MAX_EXT	256
#define _MAX_DRIVE	3
#define _MAX_DIR	MAXPATHLEN
#ifndef MAXPATH
#define MAXPATH		MAXPATHLEN
#endif
#ifndef _MAX_PATH
#define _MAX_PATH	MAXPATHLEN 
#endif

extern char **environ;   /* declaration implicit? in windows */

char* Mwultoa(unsigned long, char*, int);
char* Mwltoa(long, char*, int);
char* itoa(int, char*, int);
void perror(const char*);
void _splitpath(const char*, char*, char*, char*, char*);
void _makepath(char*, const char*, const char*, const char*, const char*);
void _searchenv(const char *, const char *, char *);
char* _fullpath(char*, const char*, size_t);
unsigned long _lrotl(unsigned long, int);
unsigned long _lrotr(unsigned long, int);
unsigned int _rotl(unsigned int, int);
unsigned int _rotr(unsigned int, int);

#ifndef MWNO_LIBC_OVERRIDE
void MwAbort ();
void MwExit (int exit_code);
void Mw_Exit (int exit_code);
void _exit (int exit_code);

#define abort MwAbort
#define exit MwExit
#define _exit Mw_Exit
#endif

MWEND_EXTERN_C_BLOCK

#endif // __STDLIB_WRAPPER_H__
#else
/***
*stdlib.h - declarations/definitions for commonly used library functions
*
* Copyright (c) 1985 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       This include file contains the function declarations for commonly
*       used library functions which either don't fit somewhere else, or,
*       cannot be declared in the normal place for other reasons.
*       [ANSI]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_STDLIB
#define _INC_STDLIB

#if !defined (_WIN32) && !defined (_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif  /* !defined (_WIN32) && !defined (_MAC) */

#ifndef _CRTBLD
/* This version of the header files is NOT for user programs.
 * It is intended for use when building the C runtimes ONLY.
 * The version intended for public use will not have this message.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef _INTERNAL_IFSTRIP_
#include <cruntime.h>
#endif  /* _INTERNAL_IFSTRIP_ */


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef CRTDLL
#define _CRTIMP __declspec(dllexport)
#else  /* CRTDLL */
#ifdef _DLL
#define _CRTIMP __declspec(dllimport)
#else  /* _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if (!defined (_MSC_VER) && !defined (__cdecl))
#define __cdecl
#endif  /* (!defined (_MSC_VER) && !defined (__cdecl)) */


#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif  /* _SIZE_T_DEFINED */


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif  /* _WCHAR_T_DEFINED */


/* Define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */


/* Definition of the argument values for the exit() function */

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1


#ifndef _ONEXIT_T_DEFINED
typedef int (__cdecl * _onexit_t)(void);
#if !__STDC__
/* Non-ANSI name for compatibility */
#define onexit_t _onexit_t
#endif  /* !__STDC__ */
#define _ONEXIT_T_DEFINED
#endif  /* _ONEXIT_T_DEFINED */


/* Data structure definitions for div and ldiv runtimes. */

#ifndef _DIV_T_DEFINED

typedef struct _div_t {
        int quot;
        int rem;
} div_t;

typedef struct _ldiv_t {
        long quot;
        long rem;
} ldiv_t;

#define _DIV_T_DEFINED
#endif  /* _DIV_T_DEFINED */


/* Maximum value that can be returned by the rand function. */

#define RAND_MAX 0x7fff

/*
 * Maximum number of bytes in multi-byte character in the current locale
 * (also defined in ctype.h).
 */
#ifndef MB_CUR_MAX
#if defined (_DLL) && defined (_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP int * __cdecl __p___mb_cur_max(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
#define MB_CUR_MAX __mb_cur_max
_CRTIMP extern int __mb_cur_max;
#endif  /* MB_CUR_MAX */


/* Minimum and maximum macros */

#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))

/*
 * Sizes for buffers used by the _makepath() and _splitpath() functions.
 * note that the sizes include space for 0-terminator
 */
#ifndef _MAC
#define _MAX_PATH   260 /* max. length of full pathname */
#define _MAX_DRIVE  3   /* max. length of drive component */
#define _MAX_DIR    256 /* max. length of path component */
#define _MAX_FNAME  256 /* max. length of file name component */
#define _MAX_EXT    256 /* max. length of extension component */
#else  /* _MAC */
#define _MAX_PATH   256 /* max. length of full pathname */
#define _MAX_DIR    32  /* max. length of path component */
#define _MAX_FNAME  64  /* max. length of file name component */
#endif  /* _MAC */

/*
 * Argument values for _set_error_mode().
 */
#define _OUT_TO_DEFAULT 0
#define _OUT_TO_STDERR  1
#define _OUT_TO_MSGBOX  2
#define _REPORT_ERRMODE 3


/* External variable declarations */

#if (defined (_MT) || defined (_DLL)) && !defined (_MAC)
_CRTIMP int * __cdecl _errno(void);
_CRTIMP unsigned long * __cdecl __doserrno(void);
#define errno       (*_errno())
#define _doserrno   (*__doserrno())
#else  /* (defined (_MT) || defined (_DLL)) && !defined (_MAC) */
_CRTIMP extern int errno;               /* XENIX style error number */
_CRTIMP extern unsigned long _doserrno; /* OS system error value */
#endif  /* (defined (_MT) || defined (_DLL)) && !defined (_MAC) */


#ifdef _MAC
_CRTIMP extern int  _macerrno;          /* OS system error value */
#endif  /* _MAC */


_CRTIMP extern char * _sys_errlist[];   /* perror error message table */
_CRTIMP extern int _sys_nerr;           /* # of entries in sys_errlist table */


#if defined (_DLL) && defined (_M_IX86)

#define __argc      (*__p___argc())     /* count of cmd line args */
#define __argv      (*__p___argv())     /* pointer to table of cmd line args */
#define __wargv     (*__p___wargv())    /* pointer to table of wide cmd line args */
#define _environ    (*__p__environ())   /* pointer to environment table */
#ifndef _MAC
#define _wenviron   (*__p__wenviron())  /* pointer to wide environment table */
#endif  /* _MAC */
#define _pgmptr     (*__p__pgmptr())    /* points to the module (EXE) name */
#ifndef _MAC
#define _wpgmptr    (*__p__wpgmptr())   /* points to the module (EXE) wide name */
#endif  /* _MAC */

_CRTIMP int *          __cdecl __p___argc(void);
_CRTIMP char ***       __cdecl __p___argv(void);
_CRTIMP wchar_t ***    __cdecl __p___wargv(void);
_CRTIMP char ***       __cdecl __p__environ(void);
_CRTIMP wchar_t ***    __cdecl __p__wenviron(void);
_CRTIMP char **        __cdecl __p__pgmptr(void);
_CRTIMP wchar_t **     __cdecl __p__wpgmptr(void);

/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP int *          __cdecl __p__fmode(void);
_CRTIMP int *          __cdecl __p__fileinfo(void);
_CRTIMP unsigned int * __cdecl __p__osver(void);
_CRTIMP unsigned int * __cdecl __p__winver(void);
_CRTIMP unsigned int * __cdecl __p__winmajor(void);
_CRTIMP unsigned int * __cdecl __p__winminor(void);

#else  /* defined (_DLL) && defined (_M_IX86) */

_CRTIMP extern int __argc;          /* count of cmd line args */
_CRTIMP extern char ** __argv;      /* pointer to table of cmd line args */
#ifndef _MAC
_CRTIMP extern wchar_t ** __wargv;  /* pointer to table of wide cmd line args */
#endif  /* _MAC */

_CRTIMP extern char ** _environ;    /* pointer to environment table */
#ifndef _MAC
_CRTIMP extern wchar_t ** _wenviron;    /* pointer to wide environment table */
#endif  /* _MAC */

_CRTIMP extern char * _pgmptr;      /* points to the module (EXE) name */
#ifndef _MAC
_CRTIMP extern wchar_t * _wpgmptr;  /* points to the module (EXE) wide name */
#endif  /* _MAC */

#endif  /* defined (_DLL) && defined (_M_IX86) */


#ifdef SPECIAL_CRTEXE
        extern int _fmode;          /* default file translation mode */
#else  /* SPECIAL_CRTEXE */
_CRTIMP extern int _fmode;          /* default file translation mode */
#endif  /* SPECIAL_CRTEXE */
_CRTIMP extern int _fileinfo;       /* open file info mode (for spawn) */


/* Windows major/minor and O.S. version numbers */

_CRTIMP extern unsigned int _osver;
_CRTIMP extern unsigned int _winver;
_CRTIMP extern unsigned int _winmajor;
_CRTIMP extern unsigned int _winminor;


/* function prototypes */

_CRTIMP void   __cdecl abort(void);
#if defined (_M_MRX000)
_CRTIMP int    __cdecl abs(int);
#else  /* defined (_M_MRX000) */
        int    __cdecl abs(int);
#endif  /* defined (_M_MRX000) */
        int    __cdecl atexit(void (__cdecl *)(void));
_CRTIMP double __cdecl atof(const char *);
_CRTIMP int    __cdecl atoi(const char *);
_CRTIMP long   __cdecl atol(const char *);
#ifdef _M_M68K
_CRTIMP long double __cdecl _atold(const char *);
#endif  /* _M_M68K */
_CRTIMP void * __cdecl bsearch(const void *, const void *, size_t, size_t,
        int (__cdecl *)(const void *, const void *));
_CRTIMP void * __cdecl calloc(size_t, size_t);
_CRTIMP div_t  __cdecl div(int, int);
_CRTIMP void   __cdecl exit(int);
_CRTIMP void   __cdecl free(void *);
_CRTIMP char * __cdecl getenv(const char *);
_CRTIMP char * __cdecl _itoa(int, char *, int);
#if _INTEGRAL_MAX_BITS >= 64   
_CRTIMP char * __cdecl _i64toa(__int64, char *, int);
_CRTIMP char * __cdecl _ui64toa(unsigned __int64, char *, int);
_CRTIMP __int64 __cdecl _atoi64(const char *);
#endif  /* _INTEGRAL_MAX_BITS >= 64    */
#if defined (_M_MRX000)
_CRTIMP long __cdecl labs(long);
#else  /* defined (_M_MRX000) */
        long __cdecl labs(long);
#endif  /* defined (_M_MRX000) */
_CRTIMP ldiv_t __cdecl ldiv(long, long);
_CRTIMP char * __cdecl _ltoa(long, char *, int);
_CRTIMP void * __cdecl malloc(size_t);
_CRTIMP int    __cdecl mblen(const char *, size_t);
_CRTIMP size_t __cdecl _mbstrlen(const char *s);
_CRTIMP int    __cdecl mbtowc(wchar_t *, const char *, size_t);
_CRTIMP size_t __cdecl mbstowcs(wchar_t *, const char *, size_t);
_CRTIMP void   __cdecl qsort(void *, size_t, size_t, int (__cdecl *)
        (const void *, const void *));
_CRTIMP int    __cdecl rand(void);
_CRTIMP void * __cdecl realloc(void *, size_t);
_CRTIMP int    __cdecl _set_error_mode(int);
_CRTIMP void   __cdecl srand(unsigned int);
_CRTIMP double __cdecl strtod(const char *, char **);
_CRTIMP long   __cdecl strtol(const char *, char **, int);
#ifdef _M_M68K
_CRTIMP long double __cdecl _strtold(const char *, char **);
#endif  /* _M_M68K */
_CRTIMP unsigned long __cdecl strtoul(const char *, char **, int);
#ifndef _MAC
_CRTIMP int    __cdecl system(const char *);
#endif  /* _MAC */
_CRTIMP char * __cdecl _ultoa(unsigned long, char *, int);
_CRTIMP int    __cdecl wctomb(char *, wchar_t);
_CRTIMP size_t __cdecl wcstombs(char *, const wchar_t *, size_t);


#ifndef _MAC
#ifndef _WSTDLIB_DEFINED

/* wide function prototypes, also declared in wchar.h  */

_CRTIMP wchar_t * __cdecl _itow (int, wchar_t *, int);
_CRTIMP wchar_t * __cdecl _ltow (long, wchar_t *, int);
_CRTIMP wchar_t * __cdecl _ultow (unsigned long, wchar_t *, int);
_CRTIMP double __cdecl wcstod(const wchar_t *, wchar_t **);
_CRTIMP long   __cdecl wcstol(const wchar_t *, wchar_t **, int);
_CRTIMP unsigned long __cdecl wcstoul(const wchar_t *, wchar_t **, int);
_CRTIMP wchar_t * __cdecl _wgetenv(const wchar_t *);
_CRTIMP int    __cdecl _wsystem(const wchar_t *);
_CRTIMP int __cdecl _wtoi(const wchar_t *);
_CRTIMP long __cdecl _wtol(const wchar_t *);
#if _INTEGRAL_MAX_BITS >= 64   
_CRTIMP wchar_t * __cdecl _i64tow(__int64, wchar_t *, int);
_CRTIMP wchar_t * __cdecl _ui64tow(unsigned __int64, wchar_t *, int);
_CRTIMP __int64   __cdecl _wtoi64(const wchar_t *);
#endif  /* _INTEGRAL_MAX_BITS >= 64    */

#define _WSTDLIB_DEFINED
#endif  /* _WSTDLIB_DEFINED */
#endif  /* _MAC */



_CRTIMP char * __cdecl _ecvt(double, int, int *, int *);
_CRTIMP void   __cdecl _exit(int);
_CRTIMP char * __cdecl _fcvt(double, int, int *, int *);
_CRTIMP char * __cdecl _fullpath(char *, const char *, size_t);
_CRTIMP char * __cdecl _gcvt(double, int, char *);
        unsigned long __cdecl _lrotl(unsigned long, int);
        unsigned long __cdecl _lrotr(unsigned long, int);
#ifndef _MAC
_CRTIMP void   __cdecl _makepath(char *, const char *, const char *, const char *,
        const char *);
#endif  /* _MAC */
        _onexit_t __cdecl _onexit(_onexit_t);
_CRTIMP void   __cdecl perror(const char *);
_CRTIMP int    __cdecl _putenv(const char *);
        unsigned int __cdecl _rotl(unsigned int, int);
        unsigned int __cdecl _rotr(unsigned int, int);
_CRTIMP void   __cdecl _searchenv(const char *, const char *, char *);
#ifndef _MAC
_CRTIMP void   __cdecl _splitpath(const char *, char *, char *, char *, char *);
#endif  /* _MAC */
_CRTIMP void   __cdecl _swab(char *, char *, int);

#ifndef _MAC
#ifndef _WSTDLIBP_DEFINED

/* wide function prototypes, also declared in wchar.h  */

_CRTIMP wchar_t * __cdecl _wfullpath(wchar_t *, const wchar_t *, size_t);
_CRTIMP void   __cdecl _wmakepath(wchar_t *, const wchar_t *, const wchar_t *, const wchar_t *,
        const wchar_t *);
_CRTIMP void   __cdecl _wperror(const wchar_t *);
_CRTIMP int    __cdecl _wputenv(const wchar_t *);
_CRTIMP void   __cdecl _wsearchenv(const wchar_t *, const wchar_t *, wchar_t *);
_CRTIMP void   __cdecl _wsplitpath(const wchar_t *, wchar_t *, wchar_t *, wchar_t *, wchar_t *);

#define _WSTDLIBP_DEFINED
#endif  /* _WSTDLIBP_DEFINED */
#endif  /* _MAC */

/* --------- The following functions are OBSOLETE --------- */
/* The Win32 API SetErrorMode, Beep and Sleep should be used instead. */
#ifndef _MAC
_CRTIMP void __cdecl _seterrormode(int);
_CRTIMP void __cdecl _beep(unsigned, unsigned);
_CRTIMP void __cdecl _sleep(unsigned long);
#endif  /* _MAC */
/* --------- The preceding functions are OBSOLETE --------- */



#if !__STDC__
/* --------- The declarations below should not be in stdlib.h --------- */
/* --------- and will be removed in a future release. Include --------- */
/* --------- ctype.h to obtain these declarations.            --------- */
#ifndef tolower
_CRTIMP int __cdecl tolower(int);
#endif  /* tolower */
#ifndef toupper
_CRTIMP int __cdecl toupper(int);
#endif  /* toupper */
/* --------- The declarations above will be removed.          --------- */
#endif  /* !__STDC__ */

#ifdef _MT
char * __cdecl _getenv_lk(const char *);                    /* _MTHREAD_ONLY */
#ifndef _MAC
wchar_t * __cdecl _wgetenv_lk(const wchar_t *);             /* _MTHREAD_ONLY */
#endif  /* _MAC */
int    __cdecl _putenv_lk(const char *);                    /* _MTHREAD_ONLY */
#ifndef _MAC
int    __cdecl _wputenv_lk(const wchar_t *);                /* _MTHREAD_ONLY */
#endif  /* _MAC */
int    __cdecl _mblen_lk(const char *, size_t);             /* _MTHREAD_ONLY */
int    __cdecl _mbtowc_lk(wchar_t*,const char*,size_t);     /* _MTHREAD_ONLY */
size_t __cdecl _mbstowcs_lk(wchar_t*,const char*,size_t);   /* _MTHREAD_ONLY */
int    __cdecl _wctomb_lk(char*,wchar_t);                   /* _MTHREAD_ONLY */
size_t __cdecl _wcstombs_lk(char*,const wchar_t*,size_t);   /* _MTHREAD_ONLY */
#else  /* _MT */
#define _getenv_lk(envvar)  getenv(envvar)                  /* _MTHREAD_ONLY */
#ifndef _MAC
#define _wgetenv_lk(envvar)  _wgetenv(envvar)               /* _MTHREAD_ONLY */
#endif  /* _MAC */
#define _putenv_lk(envvar)  _putenv(envvar)                 /* _MTHREAD_ONLY */
#ifndef _MAC
#define _wputenv_lk(envvar)  _wputenv(envvar)               /* _MTHREAD_ONLY */
#endif  /* _MAC */
#define _mblen_lk(s,n) mblen(s,n)                           /* _MTHREAD_ONLY */
#define _mbtowc_lk(pwc,s,n) mbtowc(pwc,s,n)                 /* _MTHREAD_ONLY */
#define _mbstowcs_lk(pwcs,s,n) mbstowcs(pwcs,s,n)           /* _MTHREAD_ONLY */
#define _wctomb_lk(s,wchar) wctomb(s,wchar)                 /* _MTHREAD_ONLY */
#define _wcstombs_lk(s,pwcs,n) wcstombs(s,pwcs,n)           /* _MTHREAD_ONLY */
#endif  /* _MT */

#if !__STDC__


/* Non-ANSI names for compatibility */

#ifndef __cplusplus
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif  /* __cplusplus */

#define sys_errlist _sys_errlist
#define sys_nerr    _sys_nerr
#define environ     _environ

_CRTIMP char * __cdecl ecvt(double, int, int *, int *);
_CRTIMP char * __cdecl fcvt(double, int, int *, int *);
_CRTIMP char * __cdecl gcvt(double, int, char *);
_CRTIMP char * __cdecl itoa(int, char *, int);
_CRTIMP char * __cdecl ltoa(long, char *, int);
        onexit_t __cdecl onexit(onexit_t);
_CRTIMP int    __cdecl putenv(const char *);
_CRTIMP void   __cdecl swab(char *, char *, int);
_CRTIMP char * __cdecl ultoa(unsigned long, char *, int);


#endif  /* !__STDC__ */

#ifdef __cplusplus
}

#endif  /* __cplusplus */

#ifdef _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_STDLIB */
#endif /* UNIX */
