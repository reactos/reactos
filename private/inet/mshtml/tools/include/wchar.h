/***
*wchar.h - declarations for wide character functions
*
*       Copyright (c) 1992-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the types, macros and function declarations for
*       all wide character-related functions.  They may also be declared in
*       individual header files on a functional basis.
*       [ISO]
*
*       Note: keep in sync with ctype.h, stdio.h, stdlib.h, string.h, time.h.
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#if     !defined(_M_MPPC) && !defined(_M_M68K)

#ifndef _INC_WCHAR
#define _INC_WCHAR

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef  _MSC_VER
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

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#define WCHAR_MIN	0
#define WCHAR_MAX	((wchar_t)-1)

#ifndef _WCTYPE_T_DEFINED
typedef wchar_t wint_t;
typedef wchar_t wctype_t;
#define _WCTYPE_T_DEFINED
#endif


#ifndef _VA_LIST_DEFINED
#ifdef  _M_ALPHA
typedef struct {
        char *a0;   /* pointer to first homed integer argument */
        int offset; /* byte offset of next parameter */
} va_list;
#else
typedef char *  va_list;
#endif
#define _VA_LIST_DEFINED
#endif

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif

#ifndef _FILE_DEFINED
struct _iobuf {
        char *_ptr;
        int   _cnt;
        char *_base;
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
        };
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif

/* Declare _iob[] array */

#ifndef _STDIO_DEFINED

#ifdef  _NTSDK

#ifdef  _DLL
extern FILE * _iob;
#else
extern FILE _iob[];
#endif

#else   /* ndef _NTSDK */

#if     defined(_DLL) && defined(_M_IX86)

#define _iob    (__p__iob())
_CRTIMP extern FILE * __cdecl __p__iob(void);

#else   /* !(defined(_DLL) && defined(_M_IX86)) */

_CRTIMP extern FILE _iob[];

#endif  /* defined(_DLL) && defined(_M_IX86) */

#endif  /* _NTSDK */

#endif  /* _STDIO_DEFINED */

#ifndef _FSIZE_T_DEFINED
typedef unsigned long _fsize_t; /* Could be 64 bits for Win32 */
#define _FSIZE_T_DEFINED
#endif

#ifndef _WFINDDATA_T_DEFINED

struct _wfinddata_t {
        unsigned attrib;
        time_t   time_create;   /* -1 for FAT file systems */
        time_t   time_access;   /* -1 for FAT file systems */
        time_t   time_write;
        _fsize_t size;
        wchar_t  name[260];
};

#if     !defined(_M_MPPC) && !defined(_M_M68K)
#if     _INTEGRAL_MAX_BITS >= 64
struct _wfinddatai64_t {
        unsigned attrib;
        time_t   time_create;   /* -1 for FAT file systems */
        time_t   time_access;   /* -1 for FAT file systems */
        time_t   time_write;
        __int64  size;
        wchar_t  name[260];
};
#endif
#endif

#define _WFINDDATA_T_DEFINED
#endif

/* define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

/*
 * This declaration allows the user access to the ctype look-up
 * array _ctype defined in ctype.obj by simply including ctype.h
 */

#ifdef  _NTSDK

/* definitions and declarations compatible with NT SDK */

#ifdef  _DLL
extern unsigned short * _ctype;
#define _pctype     (*_pctype_dll)
extern unsigned short **_pctype_dll;
#define _pwctype    (*_pwctype_dll)
extern unsigned short **_pwctype_dll;
#else   /* ndef _DLL */
extern unsigned short _ctype[];
extern unsigned short *_pctype;
extern wctype_t *_pwctype;
#endif  /* _DLL */

#else   /* ndef _NTSDK */

/* current declarations */

_CRTIMP extern unsigned short _ctype[];

#if     defined(_DLL) && defined(_M_IX86)

#define _pctype     (*__p__pctype())
_CRTIMP unsigned short ** __cdecl __p__pctype(void);

#define _pwctype    (*__p__pwctype())
_CRTIMP wctype_t ** __cdecl ___p__pwctype(void);

#else   /* !(defined(_DLL) && defined(_M_IX86)) */

_CRTIMP extern unsigned short *_pctype;
_CRTIMP extern wctype_t *_pwctype;

#endif  /* defined(_DLL) && defined(_M_IX86) */

#endif  /* _NTSDK */


/* set bit masks for the possible character types */

#define _UPPER      0x1 /* upper case letter */
#define _LOWER      0x2 /* lower case letter */
#define _DIGIT      0x4 /* digit[0-9] */
#define _SPACE      0x8 /* tab, carriage return, newline, */
                        /* vertical tab or form feed */
#define _PUNCT      0x10    /* punctuation character */
#define _CONTROL    0x20    /* control character */
#define _BLANK      0x40    /* space char */
#define _HEX        0x80    /* hexadecimal digit */

#define _LEADBYTE   0x8000          /* multibyte leadbyte */
#define _ALPHA      (0x0100|_UPPER|_LOWER)  /* alphabetic character */


/* Function prototypes */

#ifndef _WCTYPE_DEFINED

/* Character classification function prototypes */
/* also declared in ctype.h */

_CRTIMP int __cdecl iswalpha(wint_t);
_CRTIMP int __cdecl iswupper(wint_t);
_CRTIMP int __cdecl iswlower(wint_t);
_CRTIMP int __cdecl iswdigit(wint_t);
_CRTIMP int __cdecl iswxdigit(wint_t);
_CRTIMP int __cdecl iswspace(wint_t);
_CRTIMP int __cdecl iswpunct(wint_t);
_CRTIMP int __cdecl iswalnum(wint_t);
_CRTIMP int __cdecl iswprint(wint_t);
_CRTIMP int __cdecl iswgraph(wint_t);
_CRTIMP int __cdecl iswcntrl(wint_t);
_CRTIMP int __cdecl iswascii(wint_t);
_CRTIMP int __cdecl isleadbyte(int);

_CRTIMP wchar_t __cdecl towupper(wchar_t);
_CRTIMP wchar_t __cdecl towlower(wchar_t);

_CRTIMP int __cdecl iswctype(wint_t, wctype_t);

/* --------- The following functions are OBSOLETE --------- */
_CRTIMP int __cdecl is_wctype(wint_t, wctype_t);
/*  --------- The preceding functions are OBSOLETE --------- */

#define _WCTYPE_DEFINED
#endif

#ifndef _WDIRECT_DEFINED

/* also declared in direct.h */

_CRTIMP int __cdecl _wchdir(const wchar_t *);
_CRTIMP wchar_t * __cdecl _wgetcwd(wchar_t *, int);
_CRTIMP wchar_t * __cdecl _wgetdcwd(int, wchar_t *, int);
_CRTIMP int __cdecl _wmkdir(const wchar_t *);
_CRTIMP int __cdecl _wrmdir(const wchar_t *);

#define _WDIRECT_DEFINED
#endif

#ifndef _WIO_DEFINED

/* also declared in io.h */

_CRTIMP int __cdecl _waccess(const wchar_t *, int);
_CRTIMP int __cdecl _wchmod(const wchar_t *, int);
_CRTIMP int __cdecl _wcreat(const wchar_t *, int);
_CRTIMP long __cdecl _wfindfirst(wchar_t *, struct _wfinddata_t *);
_CRTIMP int __cdecl _wfindnext(long, struct _wfinddata_t *);
_CRTIMP int __cdecl _wunlink(const wchar_t *);
_CRTIMP int __cdecl _wrename(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wopen(const wchar_t *, int, ...);
_CRTIMP int __cdecl _wsopen(const wchar_t *, int, int, ...);
_CRTIMP wchar_t * __cdecl _wmktemp(wchar_t *);

#if     _INTEGRAL_MAX_BITS >= 64
_CRTIMP long __cdecl _wfindfirsti64(wchar_t *, struct _wfinddatai64_t *);
_CRTIMP int __cdecl _wfindnexti64(long, struct _wfinddatai64_t *);
#endif

#define _WIO_DEFINED
#endif

#ifndef _WLOCALE_DEFINED

/* wide function prototypes, also declared in wchar.h  */

_CRTIMP wchar_t * __cdecl _wsetlocale(int, const wchar_t *);

#define _WLOCALE_DEFINED
#endif

#ifndef _WPROCESS_DEFINED

/* also declared in process.h */

_CRTIMP int __cdecl _wexecl(const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _wexecle(const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _wexeclp(const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _wexeclpe(const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _wexecv(const wchar_t *, const wchar_t * const *);
_CRTIMP int __cdecl _wexecve(const wchar_t *, const wchar_t * const *, const wchar_t * const *);
_CRTIMP int __cdecl _wexecvp(const wchar_t *, const wchar_t * const *);
_CRTIMP int __cdecl _wexecvpe(const wchar_t *, const wchar_t * const *, const wchar_t * const *);
_CRTIMP int __cdecl _wspawnl(int, const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _wspawnle(int, const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _wspawnlp(int, const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _wspawnlpe(int, const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _wspawnv(int, const wchar_t *, const wchar_t * const *);
_CRTIMP int __cdecl _wspawnve(int, const wchar_t *, const wchar_t * const *,
        const wchar_t * const *);
_CRTIMP int __cdecl _wspawnvp(int, const wchar_t *, const wchar_t * const *);
_CRTIMP int __cdecl _wspawnvpe(int, const wchar_t *, const wchar_t * const *,
        const wchar_t * const *);
_CRTIMP int __cdecl _wsystem(const wchar_t *);

#define _WPROCESS_DEFINED
#endif

#ifndef _WCTYPE_INLINE_DEFINED
#ifndef __cplusplus
#define iswalpha(_c)    ( iswctype(_c,_ALPHA) )
#define iswupper(_c)    ( iswctype(_c,_UPPER) )
#define iswlower(_c)    ( iswctype(_c,_LOWER) )
#define iswdigit(_c)    ( iswctype(_c,_DIGIT) )
#define iswxdigit(_c)   ( iswctype(_c,_HEX) )
#define iswspace(_c)    ( iswctype(_c,_SPACE) )
#define iswpunct(_c)    ( iswctype(_c,_PUNCT) )
#define iswalnum(_c)    ( iswctype(_c,_ALPHA|_DIGIT) )
#define iswprint(_c)    ( iswctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT) )
#define iswgraph(_c)    ( iswctype(_c,_PUNCT|_ALPHA|_DIGIT) )
#define iswcntrl(_c)    ( iswctype(_c,_CONTROL) )
#define iswascii(_c)    ( (unsigned)(_c) < 0x80 )

#define isleadbyte(_c)  (_pctype[(unsigned char)(_c)] & _LEADBYTE)

#else	/* __cplusplus */
inline int __cdecl iswalpha(wint_t _C) {return (iswctype(_C,_ALPHA)); }
inline int __cdecl iswupper(wint_t _C) {return (iswctype(_C,_UPPER)); }
inline int __cdecl iswlower(wint_t _C) {return (iswctype(_C,_LOWER)); }
inline int __cdecl iswdigit(wint_t _C) {return (iswctype(_C,_DIGIT)); }
inline int __cdecl iswxdigit(wint_t _C) {return (iswctype(_C,_HEX)); }
inline int __cdecl iswspace(wint_t _C) {return (iswctype(_C,_SPACE)); }
inline int __cdecl iswpunct(wint_t _C) {return (iswctype(_C,_PUNCT)); }
inline int __cdecl iswalnum(wint_t _C) {return (iswctype(_C,_ALPHA|_DIGIT)); }
inline int __cdecl iswprint(wint_t _C)
	{return (iswctype(_C,_BLANK|_PUNCT|_ALPHA|_DIGIT)); }
inline int __cdecl iswgraph(wint_t _C)
	{return (iswctype(_C,_PUNCT|_ALPHA|_DIGIT)); }
inline int __cdecl iswcntrl(wint_t _C) {return (iswctype(_C,_CONTROL)); }
inline int __cdecl iswascii(wint_t _C) {return ((unsigned)(_C) < 0x80); }

inline int __cdecl isleadbyte(int _C)
	{return (_pctype[(unsigned char)(_C)] & _LEADBYTE); }
#endif	/* __cplusplus */
#define _WCTYPE_INLINE_DEFINED
#endif	/* _WCTYPE_INLINE_DEFINED */

#if !defined(_POSIX_)

/* define structure for returning status information */

#ifndef _INO_T_DEFINED

typedef unsigned short _ino_t;      /* i-node number (not used on DOS) */

#if     !__STDC__
/* Non-ANSI name for compatibility */
#ifdef  _NTSDK
#define ino_t _ino_t
#else   /* ndef _NTSDK */
typedef unsigned short ino_t;
#endif  /* _NTSDK */
#endif

#define _INO_T_DEFINED
#endif


#ifndef _DEV_T_DEFINED

#ifdef  _NTSDK
typedef short _dev_t;               /* device code */
#else   /* ndef _NTSDK */
typedef unsigned int _dev_t;        /* device code */
#endif  /* _NTSDK */

#if     !__STDC__
/* Non-ANSI name for compatibility */
#ifdef  _NTSDK
#define dev_t _dev_t
#else   /* ndef _NTSDK */
typedef unsigned int dev_t;
#endif  /* _NTSDK */
#endif

#define _DEV_T_DEFINED
#endif

#ifndef _OFF_T_DEFINED

typedef long _off_t;                /* file offset value */

#if     !__STDC__
/* Non-ANSI name for compatibility */
#ifdef  _NTSDK
#define off_t _off_t
#else   /* ndef _NTSDK */
typedef long off_t;
#endif  /* _NTSDK */
#endif

#define _OFF_T_DEFINED
#endif

#ifndef _STAT_DEFINED

struct _stat {
        _dev_t st_dev;
        _ino_t st_ino;
        unsigned short st_mode;
        short st_nlink;
        short st_uid;
        short st_gid;
        _dev_t st_rdev;
        _off_t st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
        };

#if     !__STDC__ && !defined(_NTSDK)

/* Non-ANSI names for compatibility */

struct stat {
        _dev_t st_dev;
        _ino_t st_ino;
        unsigned short st_mode;
        short st_nlink;
        short st_uid;
        short st_gid;
        _dev_t st_rdev;
        _off_t st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
        };

#endif  /* __STDC__ */

#if     _INTEGRAL_MAX_BITS >= 64
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
#endif

#define _STAT_DEFINED
#endif


#ifndef _WSTAT_DEFINED

/* also declared in stat.h */

_CRTIMP int __cdecl _wstat(const wchar_t *, struct _stat *);

#if     _INTEGRAL_MAX_BITS >= 64
_CRTIMP int __cdecl _wstati64(const wchar_t *, struct _stati64 *);
#endif

#define _WSTAT_DEFINED
#endif

#endif  /* !_POSIX_ */


#ifndef _WSTDIO_DEFINED

/* also declared in stdio.h */

#ifdef  _POSIX_
_CRTIMP FILE * __cdecl _wfsopen(const wchar_t *, const wchar_t *);
#else
_CRTIMP FILE * __cdecl _wfsopen(const wchar_t *, const wchar_t *, int);
#endif

_CRTIMP wint_t __cdecl fgetwc(FILE *);
_CRTIMP wint_t __cdecl _fgetwchar(void);
_CRTIMP wint_t __cdecl fputwc(wint_t, FILE *);
_CRTIMP wint_t __cdecl _fputwchar(wint_t);
_CRTIMP wint_t __cdecl getwc(FILE *);
_CRTIMP wint_t __cdecl getwchar(void);
_CRTIMP wint_t __cdecl putwc(wint_t, FILE *);
_CRTIMP wint_t __cdecl putwchar(wint_t);
_CRTIMP wint_t __cdecl ungetwc(wint_t, FILE *);

_CRTIMP wchar_t * __cdecl fgetws(wchar_t *, int, FILE *);
_CRTIMP int __cdecl fputws(const wchar_t *, FILE *);
_CRTIMP wchar_t * __cdecl _getws(wchar_t *);
_CRTIMP int __cdecl _putws(const wchar_t *);

_CRTIMP int __cdecl fwprintf(FILE *, const wchar_t *, ...);
_CRTIMP int __cdecl wprintf(const wchar_t *, ...);
_CRTIMP int __cdecl _snwprintf(wchar_t *, size_t, const wchar_t *, ...);
_CRTIMP int __cdecl swprintf(wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl vfwprintf(FILE *, const wchar_t *, va_list);
_CRTIMP int __cdecl vwprintf(const wchar_t *, va_list);
_CRTIMP int __cdecl _vsnwprintf(wchar_t *, size_t, const wchar_t *, va_list);
_CRTIMP int __cdecl vswprintf(wchar_t *, const wchar_t *, va_list);
_CRTIMP int __cdecl fwscanf(FILE *, const wchar_t *, ...);
_CRTIMP int __cdecl swscanf(const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl wscanf(const wchar_t *, ...);


#define getwc(_stm)     fgetwc(_stm)
#define putwc(_c,_stm)  fputwc(_c,_stm)

_CRTIMP FILE * __cdecl _wfdopen(int, const wchar_t *);
_CRTIMP FILE * __cdecl _wfopen(const wchar_t *, const wchar_t *);
_CRTIMP FILE * __cdecl _wfreopen(const wchar_t *, const wchar_t *, FILE *);
_CRTIMP void __cdecl _wperror(const wchar_t *);
_CRTIMP FILE * __cdecl _wpopen(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wremove(const wchar_t *);
_CRTIMP wchar_t * __cdecl _wtempnam(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl _wtmpnam(wchar_t *);

#define _WSTDIO_DEFINED
#endif


#ifndef _WSTDLIB_DEFINED

/* also declared in stdlib.h */

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

#ifndef _POSIX_

#ifndef _WSTDLIBP_DEFINED

/* also declared in stdlib.h  */

_CRTIMP wchar_t * __cdecl _wfullpath(wchar_t *, const wchar_t *, size_t);
_CRTIMP void   __cdecl _wmakepath(wchar_t *, const wchar_t *, const wchar_t *, const wchar_t *,
        const wchar_t *);
_CRTIMP void   __cdecl _wperror(const wchar_t *);
_CRTIMP int    __cdecl _wputenv(const wchar_t *);
_CRTIMP void   __cdecl _wsearchenv(const wchar_t *, const wchar_t *, wchar_t *);
_CRTIMP void   __cdecl _wsplitpath(const wchar_t *, wchar_t *, wchar_t *, wchar_t *, wchar_t *);

#define _WSTDLIBP_DEFINED
#endif

#endif /* _POSIX_ */


#ifndef _WSTRING_DEFINED

/* also declared in string.h */

#ifdef __cplusplus
	#define _WConst_return	const
#else
	#define _WConst_return
#endif

_CRTIMP wchar_t * __cdecl wcscat(wchar_t *, const wchar_t *);
_CRTIMP _WConst_return wchar_t * __cdecl wcschr(const wchar_t *, wchar_t);
_CRTIMP int __cdecl wcscmp(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl wcscpy(wchar_t *, const wchar_t *);
_CRTIMP size_t __cdecl wcscspn(const wchar_t *, const wchar_t *);
_CRTIMP size_t __cdecl wcslen(const wchar_t *);
_CRTIMP wchar_t * __cdecl wcsncat(wchar_t *, const wchar_t *, size_t);
_CRTIMP int __cdecl wcsncmp(const wchar_t *, const wchar_t *, size_t);
_CRTIMP wchar_t * __cdecl wcsncpy(wchar_t *, const wchar_t *, size_t);
_CRTIMP _WConst_return wchar_t * __cdecl wcspbrk(const wchar_t *, const wchar_t *);
_CRTIMP _WConst_return wchar_t * __cdecl wcsrchr(const wchar_t *, wchar_t);
_CRTIMP size_t __cdecl wcsspn(const wchar_t *, const wchar_t *);
_CRTIMP _WConst_return wchar_t * __cdecl wcsstr(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl wcstok(wchar_t *, const wchar_t *);

_CRTIMP wchar_t * __cdecl _wcsdup(const wchar_t *);
_CRTIMP int __cdecl _wcsicmp(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wcsnicmp(const wchar_t *, const wchar_t *, size_t);
_CRTIMP wchar_t * __cdecl _wcsnset(wchar_t *, wchar_t, size_t);
_CRTIMP wchar_t * __cdecl _wcsrev(wchar_t *);
_CRTIMP wchar_t * __cdecl _wcsset(wchar_t *, wchar_t);

_CRTIMP wchar_t * __cdecl _wcslwr(wchar_t *);
_CRTIMP wchar_t * __cdecl _wcsupr(wchar_t *);
_CRTIMP size_t __cdecl wcsxfrm(wchar_t *, const wchar_t *, size_t);
_CRTIMP int __cdecl wcscoll(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wcsicoll(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wcsncoll(const wchar_t *, const wchar_t *, size_t);
_CRTIMP int __cdecl _wcsnicoll(const wchar_t *, const wchar_t *, size_t);

/* Old names */
#define wcswcs wcsstr

#if     !__STDC__

#ifdef _NTSDK

/* Non-ANSI names for compatibility */
#define wcsdup  _wcsdup
#define wcsicmp _wcsicmp
#define wcsnicmp _wcsnicmp
#define wcsnset _wcsnset
#define wcsrev  _wcsrev
#define wcsset  _wcsset
#define wcslwr  _wcslwr
#define wcsupr  _wcsupr
#define wcsicoll _wcsicoll

#else /* ndef _NTSDK */

/* prototypes for oldnames.lib functions */
_CRTIMP wchar_t * __cdecl wcsdup(const wchar_t *);
_CRTIMP int __cdecl wcsicmp(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl wcsnicmp(const wchar_t *, const wchar_t *, size_t);
_CRTIMP wchar_t * __cdecl wcsnset(wchar_t *, wchar_t, size_t);
_CRTIMP wchar_t * __cdecl wcsrev(wchar_t *);
_CRTIMP wchar_t * __cdecl wcsset(wchar_t *, wchar_t);
_CRTIMP wchar_t * __cdecl wcslwr(wchar_t *);
_CRTIMP wchar_t * __cdecl wcsupr(wchar_t *);
_CRTIMP int __cdecl wcsicoll(const wchar_t *, const wchar_t *);

#endif /* ndef _NTSDK */

#endif /* !__STDC__ */

#define _WSTRING_DEFINED
#endif

#ifndef _TM_DEFINED
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

#ifndef _WTIME_DEFINED

/* also declared in time.h */

_CRTIMP wchar_t * __cdecl _wasctime(const struct tm *);
_CRTIMP wchar_t * __cdecl _wctime(const time_t *);
_CRTIMP size_t __cdecl wcsftime(wchar_t *, size_t, const wchar_t *,
        const struct tm *);
_CRTIMP wchar_t * __cdecl _wstrdate(wchar_t *);
_CRTIMP wchar_t * __cdecl _wstrtime(wchar_t *);

#define _WTIME_DEFINED
#endif



typedef int mbstate_t;
typedef wchar_t _Wint_t;

wint_t btowc(int);
size_t mbrlen(const char *, size_t, mbstate_t *);
size_t mbrtowc(wchar_t *, const char *, size_t, mbstate_t *);
size_t mbsrtowcs(wchar_t *, const char **, size_t, mbstate_t *);

size_t wcrtomb(char *, wchar_t, mbstate_t *);
size_t wcsrtombs(char *, const wchar_t **, size_t, mbstate_t *);
int wctob(wint_t);

#ifdef __cplusplus
inline int fwide(FILE *, int _M)
	{return (_M); }
inline int mbsinit(const mbstate_t *)
	{return (1); }
inline const wchar_t *wmemchr(const wchar_t *_S, wchar_t _C, size_t _N)
	{for (; 0 < _N; ++_S, --_N)
		if (*_S == _C)
			return (_S);
	return (0); }
inline int wmemcmp(const wchar_t *_S1, const wchar_t *_S2, size_t _N)
	{for (; 0 < _N; ++_S1, ++_S2, --_N)
		if (*_S1 != *_S2)
			return (*_S1 < *_S2 ? -1 : +1);
	return (0); }
inline wchar_t *wmemcpy(wchar_t *_S1, const wchar_t *_S2, size_t _N)
	{wchar_t *_Su1 = _S1;
	for (; 0 < _N; ++_Su1, ++_S2, --_N)
		*_Su1 = *_S2;
	return (_S1); }
inline wchar_t *wmemmove(wchar_t *_S1, const wchar_t *_S2, size_t _N)
	{wchar_t *_Su1 = _S1;
	if (_S2 < _Su1 && _Su1 < _S2 + _N)
		for (_Su1 += _N, _S2 += _N; 0 < _N; --_N)
			*--_Su1 = *--_S2;
	else
		for (; 0 < _N; --_N)
			*_Su1++ = *_S2++;
	return (_S1); }
inline wchar_t *wmemset(wchar_t *_S, wchar_t _C, size_t _N)
	{wchar_t *_Su = _S;
	for (; 0 < _N; ++_Su, --_N)
		*_Su = _C;
	return (_S); }
}	/* end of extern "C" */
inline wchar_t *wmemchr(wchar_t *_S, wchar_t _C, size_t _N)
	{return ((wchar_t *)wmemchr((const wchar_t *)_S, _C, _N)); }
inline wchar_t *wcschr(wchar_t *_S, wchar_t _C)
	{return ((wchar_t *)wcschr((const wchar_t *)_S, _C)); }
inline wchar_t *wcspbrk(wchar_t *_S, const wchar_t *_P)
	{return ((wchar_t *)wcspbrk((const wchar_t *)_S, _P)); }
inline wchar_t *wcsrchr(wchar_t *_S, wchar_t _C)
	{return ((wchar_t *)wcsrchr((const wchar_t *)_S, _C)); }
inline wchar_t *wcsstr(wchar_t *_S, const wchar_t *_P)
	{return ((wchar_t *)wcsstr((const wchar_t *)_S, _P)); }
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_WCHAR */

#endif /* !defined(_M_MPPC) && !defined(_M_M68K) */

