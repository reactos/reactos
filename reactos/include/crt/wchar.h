/*
 * wchar.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Defines of all functions for supporting wide characters. Actually it
 * just includes all those headers, which is not a good thing to do from a
 * processing time point of view, but it does mean that everything will be
 * in sync.
 *
 */

#ifndef	_WCHAR_H_
#define	_WCHAR_H_

/* All the headers include this file. */
#include <_mingw.h>

#ifndef RC_INVOKED

#define __need_size_t
#define __need_wint_t
#define __need_wchar_t
#define __need_NULL
#include <stddef.h>

#ifndef __VALIST
#if defined __GNUC__ && __GNUC__ >= 3
#define __need___va_list
#include <stdarg.h>
#define __VALIST __builtin_va_list
#else
#define __VALIST char*
#endif
#endif

#endif /* Not RC_INVOKED */

/*
 * MSDN says that isw* char classifications are in wchar.h and wctype.h.
 * Although the wctype names are ANSI, their exposure in this header is
 * not.
 */
#include <wctype.h>

#ifndef	__STRICT_ANSI__
/* This is necessary to support the the non-ANSI wchar declarations
   here. */
#include <sys/types.h>
#endif /* __STRICT_ANSI__ */

#define WCHAR_MIN	0
#define WCHAR_MAX	0xffff

#ifndef WEOF
#define	WEOF	(wchar_t)(0xFFFF)
#endif

#ifndef RC_INVOKED

#ifdef __cplusplus 
extern "C" {
#endif

#ifndef _FILE_DEFINED  /* Also in stdio.h */
#define	_FILE_DEFINED
typedef struct _iobuf
{
	char*	_ptr;
	int	_cnt;
	char*	_base;
	int	_flag;
	int	_file;
	int	_charbuf;
	int	_bufsiz;
	char*	_tmpfname;
} FILE;
#endif	/* Not _FILE_DEFINED */

#ifndef _TIME_T_DEFINED  /* Also in time.h */
typedef long time_t;
#define _TIME_T_DEFINED
#endif

#ifndef _TM_DEFINED /* Also in time.h */
struct tm {
        int tm_sec;     /* seconds after the minute - [0,59] */
        int tm_min;     /* minutes after the hour - [0,59] */
        int tm_hour;    /* hours since midnight - [0,23] */
        int tm_mday;    /* day of the month - [1,31] */
        int tm_mon;     /* months since January - [0,11] */
        int tm_year;    /* years since 1900 */
        int tm_wday;    /* days since Sunday - [0,6] */
        int tm_yday;    /* days since January 1 - [0,365] */
        int tm_isdst;   /* daylight savings time flag */
        };
#define _TM_DEFINED
#endif

#ifndef _WSTDIO_DEFINED
/*  Also in stdio.h - keep in sync */
_CRTIMP int __cdecl	fwprintf (FILE*, const wchar_t*, ...);
_CRTIMP int __cdecl	wprintf (const wchar_t*, ...);
_CRTIMP int __cdecl	swprintf (wchar_t*, const wchar_t*, ...);
_CRTIMP int __cdecl	_snwprintf (wchar_t*, size_t, const wchar_t*, ...);
_CRTIMP int __cdecl	vfwprintf (FILE*, const wchar_t*, __VALIST);
_CRTIMP int __cdecl	vwprintf (const wchar_t*, __VALIST);
_CRTIMP int __cdecl	vswprintf (wchar_t*, const wchar_t*, __VALIST);
_CRTIMP int __cdecl	_vsnwprintf (wchar_t*, size_t, const wchar_t*, __VALIST);
_CRTIMP int __cdecl	fwscanf (FILE*, const wchar_t*, ...);
_CRTIMP int __cdecl	wscanf (const wchar_t*, ...);
_CRTIMP int __cdecl	swscanf (const wchar_t*, const wchar_t*, ...);
_CRTIMP wint_t __cdecl	fgetwc (FILE*);
_CRTIMP wint_t __cdecl	fputwc (wchar_t, FILE*);
_CRTIMP wint_t __cdecl	ungetwc (wchar_t, FILE*);

#ifdef __MSVCRT__ 
_CRTIMP wchar_t* __cdecl fgetws (wchar_t*, int, FILE*);
_CRTIMP int __cdecl	fputws (const wchar_t*, FILE*);
_CRTIMP wint_t __cdecl	getwc (FILE*);
_CRTIMP wint_t __cdecl	getwchar (void);
_CRTIMP wint_t __cdecl	putwc (wint_t, FILE*);
_CRTIMP wint_t __cdecl	putwchar (wint_t);
#ifndef __STRICT_ANSI__
_CRTIMP wchar_t* __cdecl _getws (wchar_t*);
_CRTIMP int __cdecl	_putws (const wchar_t*);
_CRTIMP FILE* __cdecl	_wfdopen(int, wchar_t *);
_CRTIMP FILE* __cdecl	_wfopen (const wchar_t*, const wchar_t*);
_CRTIMP FILE* __cdecl	_wfreopen (const wchar_t*, const wchar_t*, FILE*);
_CRTIMP FILE* __cdecl	_wfsopen (const wchar_t*, const wchar_t*, int);
_CRTIMP wchar_t* __cdecl _wtmpnam (wchar_t*);
_CRTIMP wchar_t* __cdecl _wtempnam (const wchar_t*, const wchar_t*);
_CRTIMP int __cdecl	_wrename (const wchar_t*, const wchar_t*);
_CRTIMP int __cdecl	_wremove (const wchar_t*);
_CRTIMP void __cdecl	_wperror (const wchar_t*);
_CRTIMP FILE* __cdecl	_wpopen (const wchar_t*, const wchar_t*);
#endif  /* __STRICT_ANSI__ */
#endif	/* __MSVCRT__ */

#ifndef __NO_ISOCEXT  /* externs in libmingwex.a */
int __cdecl snwprintf (wchar_t* s, size_t n, const wchar_t*  format, ...);
__CRT_INLINE int __cdecl
vsnwprintf (wchar_t* s, size_t n, const wchar_t* format, __VALIST arg)
  { return _vsnwprintf ( s, n, format, arg);}
int __cdecl vwscanf (const wchar_t * __restrict__, __VALIST);
int __cdecl vfwscanf (FILE * __restrict__,
		       const wchar_t * __restrict__, __VALIST);
int __cdecl vswscanf (const wchar_t * __restrict__,
		       const wchar_t * __restrict__, __VALIST);
#endif

#define _WSTDIO_DEFINED
#endif /* _WSTDIO_DEFINED */

#ifndef _WSTDLIB_DEFINED /* also declared in stdlib.h */
_CRTIMP long __cdecl 		wcstol (const wchar_t*, wchar_t**, int);
_CRTIMP unsigned long __cdecl	wcstoul (const wchar_t*, wchar_t**, int);
_CRTIMP double __cdecl		wcstod (const wchar_t*, wchar_t**);
#if !defined __NO_ISOCEXT /* in libmingwex.a */
float __cdecl wcstof (const wchar_t * __restrict__, wchar_t ** __restrict__);
long double __cdecl wcstold (const wchar_t * __restrict__, wchar_t ** __restrict__);
#endif /* __NO_ISOCEXT */
#define  _WSTDLIB_DEFINED
#endif /* _WSTDLIB_DEFINED */

#ifndef _WTIME_DEFINED
#ifndef __STRICT_ANSI__
#ifdef __MSVCRT__
/* wide function prototypes, also declared in time.h */
_CRTIMP wchar_t* __cdecl	_wasctime (const struct tm*);
_CRTIMP wchar_t* __cdecl	_wctime (const time_t*);
_CRTIMP wchar_t* __cdecl	_wstrdate (wchar_t*);
_CRTIMP wchar_t* __cdecl	_wstrtime (wchar_t*);
#if __MSVCRT_VERSION__ >= 0x601
_CRTIMP wchar_t* __cdecl	_wctime64 (const __time64_t*);
#endif
#endif /* __MSVCRT__ */
#endif /* __STRICT_ANSI__ */
_CRTIMP size_t __cdecl	wcsftime (wchar_t*, size_t, const wchar_t*, const struct tm*);
#define _WTIME_DEFINED
#endif /* _WTIME_DEFINED */ 


#ifndef _WSTRING_DEFINED
/*
 * Unicode versions of the standard string calls.
 * Also in string.h.
 */
_CRTIMP wchar_t* __cdecl wcscat (wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl wcschr (const wchar_t*, wchar_t);
_CRTIMP int __cdecl	wcscmp (const wchar_t*, const wchar_t*);
_CRTIMP int __cdecl	wcscoll (const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl wcscpy (wchar_t*, const wchar_t*);
_CRTIMP size_t __cdecl	wcscspn (const wchar_t*, const wchar_t*);
/* Note:  _wcserror requires __MSVCRT_VERSION__ >= 0x0700.  */
_CRTIMP size_t __cdecl	wcslen (const wchar_t*);
_CRTIMP wchar_t* __cdecl wcsncat (wchar_t*, const wchar_t*, size_t);
_CRTIMP int __cdecl	wcsncmp(const wchar_t*, const wchar_t*, size_t);
_CRTIMP wchar_t* __cdecl wcsncpy(wchar_t*, const wchar_t*, size_t);
_CRTIMP wchar_t* __cdecl wcspbrk(const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl wcsrchr(const wchar_t*, wchar_t);
_CRTIMP size_t __cdecl	wcsspn(const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl wcsstr(const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl wcstok(wchar_t*, const wchar_t*);
_CRTIMP size_t __cdecl	wcsxfrm(wchar_t*, const wchar_t*, size_t);

#ifndef	__STRICT_ANSI__
/*
 * Unicode versions of non-ANSI functions provided by CRTDLL.
 */

/* NOTE: _wcscmpi not provided by CRTDLL, this define is for portability */
#define		_wcscmpi	_wcsicmp

_CRTIMP wchar_t* __cdecl _wcsdup (const wchar_t*);
_CRTIMP int __cdecl	_wcsicmp (const wchar_t*, const wchar_t*);
_CRTIMP int __cdecl	_wcsicoll (const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl _wcslwr (wchar_t*);
_CRTIMP int __cdecl	_wcsnicmp (const wchar_t*, const wchar_t*, size_t);
_CRTIMP wchar_t* __cdecl _wcsnset (wchar_t*, wchar_t, size_t);
_CRTIMP wchar_t* __cdecl _wcsrev (wchar_t*);
_CRTIMP wchar_t* __cdecl _wcsset (wchar_t*, wchar_t);
_CRTIMP wchar_t* __cdecl _wcsupr (wchar_t*);

#ifdef __MSVCRT__
_CRTIMP int __cdecl  _wcsncoll(const wchar_t*, const wchar_t*, size_t);
_CRTIMP int   __cdecl _wcsnicoll(const wchar_t*, const wchar_t*, size_t);
#if __MSVCRT_VERSION__ >= 0x0700
_CRTIMP  wchar_t* __cdecl _wcserror(int);
_CRTIMP  wchar_t* __cdecl __wcserror(const wchar_t*);
#endif
#endif

#ifndef	_NO_OLDNAMES
/* NOTE: There is no _wcscmpi, but this is for compatibility. */
__CRT_INLINE int __cdecl
wcscmpi (const wchar_t * __ws1, const wchar_t * __ws2)
  {return _wcsicmp (__ws1, __ws2);}
_CRTIMP wchar_t* __cdecl wcsdup (const wchar_t*);
_CRTIMP int __cdecl	wcsicmp (const wchar_t*, const wchar_t*);
_CRTIMP int __cdecl	wcsicoll (const wchar_t*, const wchar_t*);
_CRTIMP wchar_t* __cdecl wcslwr (wchar_t*);
_CRTIMP int __cdecl	wcsnicmp (const wchar_t*, const wchar_t*, size_t);
_CRTIMP wchar_t* __cdecl wcsnset (wchar_t*, wchar_t, size_t);
_CRTIMP wchar_t* __cdecl wcsrev (wchar_t*);
_CRTIMP wchar_t* __cdecl wcsset (wchar_t*, wchar_t);
_CRTIMP wchar_t* __cdecl wcsupr (wchar_t*);
#endif	/* Not _NO_OLDNAMES */

#endif	/* Not strict ANSI */

#define _WSTRING_DEFINED
#endif  /* _WSTRING_DEFINED */

/* These are resolved by -lmingwex. Alternatively, they can be resolved by
   adding -lmsvcp60 to your command line, which will give you the VC++
   versions of these functions. If you want the latter and don't have
   msvcp60.dll in your windows system directory, you can easily obtain
   it with a search from your favorite search engine.  */
#ifndef __STRICT_ANSI__
typedef wchar_t _Wint_t;
#endif

typedef int mbstate_t;

wint_t __cdecl btowc(int);
size_t __cdecl mbrlen(const char * __restrict__, size_t,
		      mbstate_t * __restrict__);
size_t __cdecl mbrtowc(wchar_t * __restrict__, const char * __restrict__,
		       size_t, mbstate_t * __restrict__);
size_t __cdecl mbsrtowcs(wchar_t * __restrict__, const char ** __restrict__,
			 size_t, mbstate_t * __restrict__);
size_t __cdecl wcrtomb(char * __restrict__, wchar_t,
		       mbstate_t * __restrict__);
size_t __cdecl wcsrtombs(char * __restrict__, const wchar_t ** __restrict__,
			 size_t, mbstate_t * __restrict__);
int __cdecl wctob(wint_t);

#ifndef __NO_ISOCEXT /* these need static lib libmingwex.a */
__CRT_INLINE int __cdecl fwide(FILE* __UNUSED_PARAM(stream),
			       int __UNUSED_PARAM(mode))
  {return -1;} /* limited to byte orientation */ 
__CRT_INLINE int __cdecl mbsinit(const mbstate_t* __UNUSED_PARAM(ps))
  {return 1;}
wchar_t* __cdecl wmemset(wchar_t *, wchar_t, size_t);
wchar_t* __cdecl wmemchr(const wchar_t*, wchar_t, size_t);
int wmemcmp(const wchar_t*, const wchar_t *, size_t);
wchar_t* __cdecl wmemcpy(wchar_t* __restrict__,
		         const wchar_t* __restrict__,
			 size_t);
wchar_t* __cdecl wmemmove(wchar_t* s1, const wchar_t *, size_t);
long long __cdecl wcstoll(const wchar_t * __restrict__,
			  wchar_t** __restrict__, int);
unsigned long long __cdecl wcstoull(const wchar_t * __restrict__,
			    wchar_t ** __restrict__, int);
#endif /* __NO_ISOCEXT */

#ifndef	__STRICT_ANSI__
/* non-ANSI wide char functions from io.h, direct.h, sys/stat.h and locale.h.  */

#ifndef	_FSIZE_T_DEFINED
typedef	unsigned long	_fsize_t;
#define _FSIZE_T_DEFINED
#endif

#ifndef _WFINDDATA_T_DEFINED
struct _wfinddata_t {
	unsigned	attrib;
	time_t		time_create;	/* -1 for FAT file systems */
	time_t		time_access;	/* -1 for FAT file systems */
	time_t		time_write;
	_fsize_t	size;
	wchar_t		name[260];	/* may include spaces. */
};
struct _wfinddatai64_t {
	unsigned    attrib;
	time_t      time_create;
	time_t      time_access;
	time_t      time_write;
	__int64     size;
	wchar_t     name[260];
};
struct __wfinddata64_t {
        unsigned    attrib;
        __time64_t  time_create;    
        __time64_t  time_access;
        __time64_t  time_write;
        _fsize_t    size;
        wchar_t     name[260];
};
#define _WFINDDATA_T_DEFINED
#endif

/* Wide character versions. Also defined in io.h. */
/* CHECK: I believe these only exist in MSVCRT, and not in CRTDLL. Also
   applies to other wide character versions? */
#if !defined (_WIO_DEFINED)
#if defined (__MSVCRT__)
#include <stdint.h>  /* For intptr_t.  */
_CRTIMP int __cdecl	_waccess (const wchar_t*, int);
_CRTIMP int __cdecl	_wchmod (const wchar_t*, int);
_CRTIMP int __cdecl	_wcreat (const wchar_t*, int);
_CRTIMP long __cdecl	_wfindfirst (const wchar_t*, struct _wfinddata_t *);
_CRTIMP int __cdecl	_wfindnext (long, struct _wfinddata_t *);
_CRTIMP int __cdecl	_wunlink (const wchar_t*);
_CRTIMP int __cdecl	_wopen (const wchar_t*, int, ...);
_CRTIMP int __cdecl	_wsopen (const wchar_t*, int, int, ...);
_CRTIMP wchar_t* __cdecl _wmktemp (wchar_t*);
_CRTIMP long __cdecl	_wfindfirsti64 (const wchar_t*, struct _wfinddatai64_t*);
_CRTIMP int __cdecl 	_wfindnexti64 (long, struct _wfinddatai64_t*);
#if __MSVCRT_VERSION__ >= 0x0601
_CRTIMP intptr_t __cdecl _wfindfirst64(const wchar_t*, struct __wfinddata64_t*); 
_CRTIMP intptr_t __cdecl _wfindnext64(intptr_t, struct __wfinddata64_t*);
#endif /* __MSVCRT_VERSION__ >= 0x0601 */
#endif /* defined (__MSVCRT__) */
#define _WIO_DEFINED
#endif /* _WIO_DEFINED */

#ifndef _WDIRECT_DEFINED
/* Also in direct.h */
#ifdef __MSVCRT__ 
_CRTIMP int __cdecl	  _wchdir (const wchar_t*);
_CRTIMP wchar_t* __cdecl  _wgetcwd (wchar_t*, int);
_CRTIMP wchar_t* __cdecl  _wgetdcwd (int, wchar_t*, int);
_CRTIMP int __cdecl	  _wmkdir (const wchar_t*);
_CRTIMP int __cdecl	  _wrmdir (const wchar_t*);
#endif	/* __MSVCRT__ */
#define _WDIRECT_DEFINED
#endif /* _WDIRECT_DEFINED */

#ifndef _STAT_DEFINED
/*
 * The structure manipulated and returned by stat and fstat.
 *
 * NOTE: If called on a directory the values in the time fields are not only
 * invalid, they will cause localtime et. al. to return NULL. And calling
 * asctime with a NULL pointer causes an Invalid Page Fault. So watch it!
 */
struct _stat
{
	_dev_t	st_dev;		/* Equivalent to drive number 0=A 1=B ... */
	_ino_t	st_ino;		/* Always zero ? */
	_mode_t	st_mode;	/* See above constants */
	short	st_nlink;	/* Number of links. */
	short	st_uid;		/* User: Maybe significant on NT ? */
	short	st_gid;		/* Group: Ditto */
	_dev_t	st_rdev;	/* Seems useless (not even filled in) */
	_off_t	st_size;	/* File size in bytes */
	time_t	st_atime;	/* Accessed date (always 00:00 hrs local
				 * on FAT) */
	time_t	st_mtime;	/* Modified time */
	time_t	st_ctime;	/* Creation time */
};

#ifndef	_NO_OLDNAMES
/* NOTE: Must be the same as _stat above. */
struct stat
{
	_dev_t	st_dev;		/* Equivalent to drive number 0=A 1=B ... */
	_ino_t	st_ino;		/* Always zero ? */
	_mode_t	st_mode;	/* See above constants */
	short	st_nlink;	/* Number of links. */
	short	st_uid;		/* User: Maybe significant on NT ? */
	short	st_gid;		/* Group: Ditto */
	_dev_t	st_rdev;	/* Seems useless (not even filled in) */
	_off_t	st_size;	/* File size in bytes */
	time_t	st_atime;	/* Accessed date (always 00:00 hrs local
				 * on FAT) */
	time_t	st_mtime;	/* Modified time */
	time_t	st_ctime;	/* Creation time */
};
#endif /* _NO_OLDNAMES */

#if defined (__MSVCRT__)
struct _stati64 {
    _dev_t st_dev;
    _ino_t st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    __int64 st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
    };

struct __stat64
{
    _dev_t st_dev;
    _ino_t st_ino;
    _mode_t st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    __int64 st_size;
    __time64_t st_atime;
    __time64_t st_mtime;
    __time64_t st_ctime;
};
#endif  /* __MSVCRT__ */
#define _STAT_DEFINED
#endif /* _STAT_DEFINED */

#if !defined ( _WSTAT_DEFINED)
/* also declared in sys/stat.h */
#if defined __MSVCRT__
_CRTIMP int __cdecl	_wstat (const wchar_t*, struct _stat*);
_CRTIMP int __cdecl	_wstati64 (const wchar_t*, struct _stati64*);
#if __MSVCRT_VERSION__ >= 0x0601
_CRTIMP int __cdecl _wstat64 (const wchar_t*, struct __stat64*);
#endif /* __MSVCRT_VERSION__ >= 0x0601 */
#endif  /* __MSVCRT__ */
#define _WSTAT_DEFINED
#endif /* ! _WSTAT_DEFIND  */

#ifndef _WLOCALE_DEFINED  /* also declared in locale.h */
_CRTIMP wchar_t* __cdecl _wsetlocale (int, const wchar_t*);
#define _WLOCALE_DEFINED
#endif

#endif /* not __STRICT_ANSI__ */

#ifdef __cplusplus
}	/* end of extern "C" */
#endif

#endif /* Not RC_INVOKED */

#endif /* not _WCHAR_H_ */

