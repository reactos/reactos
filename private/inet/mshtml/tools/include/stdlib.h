/***
*stdlib.h - declarations/definitions for commonly used library functions
*
*       Copyright (c) 1985-1996, Microsoft Corporation. All rights reserved.
*
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
#endif

#ifndef _INC_STDLIB
#define _INC_STDLIB

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else   /* ndef _NTSDK */
/* current definition */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* _NTSDK */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif


/* Define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


/* Definition of the argument values for the exit() function */

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1


#ifndef _ONEXIT_T_DEFINED
typedef int (__cdecl * _onexit_t)(void);
#if     !__STDC__
/* Non-ANSI name for compatibility */
#define onexit_t _onexit_t
#endif
#define _ONEXIT_T_DEFINED
#endif


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
#endif

/* Maximum value that can be returned by the rand function. */

#define RAND_MAX 0x7fff

/*
 * Maximum number of bytes in multi-byte character in the current locale
 * (also defined in ctype.h).
 */
#ifndef MB_CUR_MAX

#ifdef  _NTSDK

/* definition compatible with NT SDK */
#ifdef  _DLL
#define __mb_cur_max    (*__mb_cur_max_dll)
#define MB_CUR_MAX      (*__mb_cur_max_dll)
extern  int *__mb_cur_max_dll;
#else   /* ndef _DLL */
#define MB_CUR_MAX      __mb_cur_max
extern  int __mb_cur_max;
#endif  /* _DLL */

#else   /* ndef _NTSDK */

/* current definition */
#if     defined(_DLL) && defined(_M_IX86)
#define MB_CUR_MAX  (*__p___mb_cur_max())
_CRTIMP int * __cdecl __p___mb_cur_max(void);
#else   /* !(defined(_DLL) && defined(_M_IX86)) */
#define MB_CUR_MAX __mb_cur_max
_CRTIMP extern int __mb_cur_max;
#endif  /* defined(_DLL) && defined(_M_IX86) */

#endif  /* _NTSDK */

#endif  /* MB_CUR_MAX */


/* Minimum and maximum macros */

#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))

/*
 * Sizes for buffers used by the _makepath() and _splitpath() functions.
 * note that the sizes include space for 0-terminator
 */
#if !defined(_M_MPPC) && !defined(_M_M68K)
#define _MAX_PATH   260 /* max. length of full pathname */
#define _MAX_DRIVE  3   /* max. length of drive component */
#define _MAX_DIR    256 /* max. length of path component */
#define _MAX_FNAME  256 /* max. length of file name component */
#define _MAX_EXT    256 /* max. length of extension component */
#else   /* defined(_M_M68K) || defined(_M_MPPC) */
#define _MAX_PATH   256 /* max. length of full pathname */
#define _MAX_DIR    32  /* max. length of path component */
#define _MAX_FNAME  64  /* max. length of file name component */
#endif  /* defined(_M_M68K) || defined(_M_MPPC) */

/*
 * Argument values for _set_error_mode().
 */
#define _OUT_TO_DEFAULT 0
#define _OUT_TO_STDERR  1
#define _OUT_TO_MSGBOX  2
#define _REPORT_ERRMODE 3

/* External variable declarations */

#if (defined(_MT) || defined(_DLL)) && (!defined(_M_MPPC) && !defined(_M_M68K))
_CRTIMP int * __cdecl _errno(void);
_CRTIMP unsigned long * __cdecl __doserrno(void);
#define errno       (*_errno())
#define _doserrno   (*__doserrno())
#else   /* ndef _MT && ndef _DLL */
_CRTIMP extern int errno;               /* XENIX style error number */
_CRTIMP extern unsigned long _doserrno; /* OS system error value */
#endif  /* _MT || _DLL */

#if defined(_M_MPPC) || defined(_M_M68K)
_CRTIMP extern int  _macerrno;  /* OS system error value */
#endif

#ifdef  _NTSDK

/* Definitions and declarations compatible with the NT SDK */

#ifdef  _DLL

extern char ** _sys_errlist;    /* perror error message table */

#define _sys_nerr   (*_sys_nerr_dll)
#define __argc      (*__argc_dll)
#define __argv      (*__argv_dll)
#ifndef _MAC
#define __wargv     (*__wargv_dll)
#endif /* ndef _MAC */
#define _environ    (*_environ_dll)
#ifndef _MAC
#define _wenviron   (*_wenviron_dll)
#endif /* ndef _MAC */
#define _fmode      (*_fmode_dll)
#define _fileinfo   (*_fileinfo_dll)

extern int * _sys_nerr_dll;     /* # of entries in sys_errlist table */
extern int * __argc_dll;        /* count of cmd line args */
extern char *** __argv_dll;     /* pointer to table of cmd line args */
#ifndef _MAC
extern wchar_t *** __wargv_dll; /* pointer to table of wide cmd line args */
#endif /* ndef _MAC */
extern char *** _environ_dll;   /* pointer to environment table */
#ifndef _MAC
extern wchar_t *** _wenviron_dll;   /* pointer to wide environment table */
#endif /* ndef _MAC */
extern int * _fmode_dll;        /* default file translation mode */
extern int * _fileinfo_dll;     /* open file info mode (for spawn) */

#define _pgmptr     (*_pgmptr_dll)
#ifndef _MAC
#define _wpgmptr    (*_wpgmptr_dll)
#endif /* ndef _MAC */

#define _osver      (*_osver_dll)
#define _winver     (*_winver_dll)
#define _winmajor   (*_winmajor_dll)
#define _winminor   (*_winminor_dll)

extern char ** _pgmptr_dll;
#ifndef _MAC
extern wchar_t ** _wpgmptr_dll;
#endif /* ndef _MAC */

extern unsigned int * _osver_dll;
extern unsigned int * _winver_dll;
extern unsigned int * _winmajor_dll;
extern unsigned int * _winminor_dll;

#else   /* ndef _DLL */


extern char * _sys_errlist[];   /* perror error message table */
extern int _sys_nerr;           /* # of entries in sys_errlist table */

extern int __argc;              /* count of cmd line args */
extern char ** __argv;          /* pointer to table of cmd line args */
#ifndef _MAC
extern wchar_t ** __wargv;      /* pointer to table of wide cmd line args */
#endif  /* ndef _MAC */

#ifdef _POSIX_
extern char ** environ;         /* pointer to environment table */
#else
extern char **    _environ;     /* pointer to environment table */
#ifndef _MAC
extern wchar_t ** _wenviron;    /* pointer to wide environment table */
#endif  /* ndef _MAC */
#endif

extern int _fmode;              /* default file translation mode */
extern int _fileinfo;           /* open file info mode (for spawn) */

extern char * _pgmptr;          /* points to the module (EXE) name */
#ifndef _MAC
extern wchar_t * _wpgmptr;      /* points to the module (EXE) wide name */
#endif  /* ndef _MAC */

/* Windows major/minor and O.S. version numbers */

extern unsigned int _osver;
extern unsigned int _winver;
extern unsigned int _winmajor;
extern unsigned int _winminor;

#endif  /* _DLL */

#else   /* ndef _NTSDK */

/* Current definitions and declarations */

_CRTIMP extern char * _sys_errlist[];   /* perror error message table */
_CRTIMP extern int _sys_nerr;   /* # of entries in sys_errlist table */

#if defined(_DLL) && defined(_M_IX86)

#define __argc      (*__p___argc())     /* count of cmd line args */
#define __argv      (*__p___argv())     /* pointer to table of cmd line args */
#define __wargv     (*__p___wargv())    /* pointer to table of wide cmd line args */
#define _environ    (*__p__environ())   /* pointer to environment table */
#ifdef _POSIX_
extern char ** environ;                 /* pointer to environment table */
#else
#ifndef _MAC
#define _wenviron   (*__p__wenviron())  /* pointer to wide environment table */
#endif  /* ndef _MAC */
#endif /* _POSIX_ */
#define _fmode      (*__p__fmode())
#define _fileinfo   (*__p__fileinfo())
#define _osver      (*__p__osver())
#define _pgmptr     (*__p__pgmptr())    /* points to the module (EXE) name */
#ifndef _MAC
#define _wpgmptr    (*__p__wpgmptr())   /* points to the module (EXE) wide name */
#endif  /* ndef _MAC */
#define _winver     (*__p__winver())
#define _winmajor   (*__p__winmajor())
#define _winminor   (*__p__winminor())

_CRTIMP int *          __cdecl __p___argc(void);
_CRTIMP char ***       __cdecl __p___argv(void);
#ifndef _MAC
_CRTIMP wchar_t ***    __cdecl __p___wargv(void);
#endif  /* ndef _MAC */
_CRTIMP char ***       __cdecl __p__environ(void);
#ifndef _MAC
_CRTIMP wchar_t ***    __cdecl __p__wenviron(void);
#endif  /* ndef _MAC */
_CRTIMP int *          __cdecl __p__fmode(void);
_CRTIMP int *          __cdecl __p__fileinfo(void);
_CRTIMP unsigned int * __cdecl __p__osver(void);
_CRTIMP char **        __cdecl __p__pgmptr(void);
#ifndef _MAC
_CRTIMP wchar_t **     __cdecl __p__wpgmptr(void);
#endif  /* ndef _MAC */
_CRTIMP unsigned int * __cdecl __p__winver(void);
_CRTIMP unsigned int * __cdecl __p__winmajor(void);
_CRTIMP unsigned int * __cdecl __p__winminor(void);

#else   /* !(defined(_DLL) && defined(_M_IX86)) */
_CRTIMP extern int __argc;          /* count of cmd line args */
_CRTIMP extern char ** __argv;      /* pointer to table of cmd line args */
#ifndef _MAC
_CRTIMP extern wchar_t ** __wargv;  /* pointer to table of wide cmd line args */
#endif  /* ndef _MAC */

#ifdef _POSIX_
extern char ** environ;                 /* pointer to environment table */
#else
_CRTIMP extern char **    _environ; /* pointer to environment table */
#ifndef _MAC
_CRTIMP extern wchar_t ** _wenviron;    /* pointer to wide environment table */
#endif  /* ndef _MAC */
#endif  /* _POSIX_ */

_CRTIMP extern int _fmode;          /* default file translation mode */
_CRTIMP extern int _fileinfo;       /* open file info mode (for spawn) */

_CRTIMP extern char * _pgmptr;      /* points to the module (EXE) name */
#ifndef _MAC
_CRTIMP extern wchar_t * _wpgmptr;  /* points to the module (EXE) wide name */
#endif  /* ndef _MAC */

/* Windows major/minor and O.S. version numbers */

_CRTIMP extern unsigned int _osver;
_CRTIMP extern unsigned int _winver;
_CRTIMP extern unsigned int _winmajor;
_CRTIMP extern unsigned int _winminor;
#endif  /* defined(_DLL) && defined(_M_IX86) */

#endif  /* _NTSDK */

/* function prototypes */

_CRTIMP void   __cdecl abort(void);
#if defined(_M_MRX000)
_CRTIMP int    __cdecl abs(int);

#else
        int    __cdecl abs(int);
#endif
        int    __cdecl atexit(void (__cdecl *)(void));
_CRTIMP double __cdecl atof(const char *);
_CRTIMP int    __cdecl atoi(const char *);
_CRTIMP long   __cdecl atol(const char *);
#ifdef _M_M68K
_CRTIMP long double __cdecl _atold(const char *);
#endif
_CRTIMP void * __cdecl bsearch(const void *, const void *, size_t, size_t,
        int (__cdecl *)(const void *, const void *));
_CRTIMP void * __cdecl calloc(size_t, size_t);
_CRTIMP div_t  __cdecl div(int, int);
_CRTIMP void   __cdecl exit(int);
_CRTIMP void   __cdecl free(void *);
_CRTIMP char * __cdecl getenv(const char *);
_CRTIMP char * __cdecl _itoa(int, char *, int);
#if     _INTEGRAL_MAX_BITS >= 64
_CRTIMP char * __cdecl _i64toa(__int64, char *, int);
_CRTIMP char * __cdecl _ui64toa(unsigned __int64, char *, int);
_CRTIMP __int64 __cdecl _atoi64(const char *);
#endif
#if defined(_M_MRX000)
_CRTIMP long __cdecl labs(long);
#else
        long __cdecl labs(long);
#endif
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
#endif
_CRTIMP unsigned long __cdecl strtoul(const char *, char **, int);
#if !defined(_M_MPPC) && !defined(_M_M68K)
_CRTIMP int    __cdecl system(const char *);
#endif
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
#if     _INTEGRAL_MAX_BITS >= 64
_CRTIMP wchar_t * __cdecl _i64tow(__int64, wchar_t *, int);
_CRTIMP wchar_t * __cdecl _ui64tow(unsigned __int64, wchar_t *, int);
_CRTIMP __int64   __cdecl _wtoi64(const wchar_t *);
#endif

#define _WSTDLIB_DEFINED
#endif
#endif  /* ndef _MAC */

#ifndef _POSIX_

_CRTIMP char * __cdecl _ecvt(double, int, int *, int *);
_CRTIMP void   __cdecl _exit(int);
_CRTIMP char * __cdecl _fcvt(double, int, int *, int *);
_CRTIMP char * __cdecl _fullpath(char *, const char *, size_t);
_CRTIMP char * __cdecl _gcvt(double, int, char *);
        unsigned long __cdecl _lrotl(unsigned long, int);
        unsigned long __cdecl _lrotr(unsigned long, int);
#if !defined(_M_MPPC) && !defined(_M_M68K)
_CRTIMP void   __cdecl _makepath(char *, const char *, const char *, const char *,
        const char *);
#endif
        _onexit_t __cdecl _onexit(_onexit_t);
_CRTIMP void   __cdecl perror(const char *);
_CRTIMP int    __cdecl _putenv(const char *);
        unsigned int __cdecl _rotl(unsigned int, int);
        unsigned int __cdecl _rotr(unsigned int, int);
_CRTIMP void   __cdecl _searchenv(const char *, const char *, char *);
#if !defined(_M_MPPC) && !defined(_M_M68K)
_CRTIMP void   __cdecl _splitpath(const char *, char *, char *, char *, char *);
#endif
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
#endif
#endif  /* ndef _MAC */

/* --------- The following functions are OBSOLETE --------- */
/* The Win32 API SetErrorMode, Beep and Sleep should be used instead. */
#if     !defined(_M_MPPC) && !defined(_M_M68K)
_CRTIMP void __cdecl _seterrormode(int);
_CRTIMP void __cdecl _beep(unsigned, unsigned);
_CRTIMP void __cdecl _sleep(unsigned long);
#endif  /* ndef defined(_M_M68K) || defined(_M_MPPC) */
/* --------- The preceding functions are OBSOLETE --------- */

#endif  /* _POSIX_ */

#if     !__STDC__
/* --------- The declarations below should not be in stdlib.h --------- */
/* --------- and will be removed in a future release. Include --------- */
/* --------- ctype.h to obtain these declarations.            --------- */
#ifndef tolower     /* tolower has been undefined - use function */
_CRTIMP int __cdecl tolower(int);
#endif  /* tolower */
#ifndef toupper     /* toupper has been undefined - use function */
_CRTIMP int __cdecl toupper(int);
#endif  /* toupper */
/* --------- The declarations above will be removed.          --------- */
#endif


#if     !__STDC__

#ifndef _POSIX_

/* Non-ANSI names for compatibility */

#ifdef  _NTSDK

#ifndef __cplusplus
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#define sys_errlist _sys_errlist
#define sys_nerr    _sys_nerr
#define environ     _environ

#define DOS_MODE    _DOS_MODE
#define OS2_MODE    _OS2_MODE

#define ecvt        _ecvt
#define fcvt        _fcvt
#define gcvt        _gcvt
#define itoa        _itoa
#define ltoa        _ltoa
#define onexit      _onexit
#define putenv      _putenv
#define swab        _swab
#define ultoa       _ultoa

#else   /* ndef _NTSDK */

#ifndef __cplusplus
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

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

#endif  /* _NTSDK */

#endif  /* _POSIX_ */

#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif


#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_STDLIB */
