/***
*stdio.h - definitions/declarations for standard I/O routines
*
* Copyright (c) 1985 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       This file defines the structures, values, macros, and functions
*       used by the level 2 I/O ("standard I/O") routines.
*       [ANSI/System V]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */
#ifndef UNIX

#ifndef _INC_STDIO
#define _INC_STDIO

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


#ifndef _MAC
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif  /* _WCHAR_T_DEFINED */
#ifndef _WCTYPE_T_DEFINED 
typedef wchar_t wint_t;
typedef wchar_t wctype_t;
#define _WCTYPE_T_DEFINED

#endif /* _WCTYPE_T_DEFINED */

#endif  /* _MAC */


#ifndef _VA_LIST_DEFINED
#ifdef _M_ALPHA
typedef struct {
        char *a0;       /* pointer to first homed integer argument */
        int offset;     /* byte offset of next parameter */
} va_list;
#else  /* _M_ALPHA */
typedef char *  va_list;
#endif  /* _M_ALPHA */
#define _VA_LIST_DEFINED
#endif  /* _VA_LIST_DEFINED */


/* Buffered I/O macros */

#if defined (_M_MPPC)
#define BUFSIZ  4096
#else  /* defined (_M_MPPC) */
#define BUFSIZ  512
#endif  /* defined (_M_MPPC) */

#ifndef _INTERNAL_IFSTRIP_
/*
 * Real default size for stdio buffers
 */
#define _INTERNAL_BUFSIZ    4096
#define _SMALL_BUFSIZ       512
#endif  /* _INTERNAL_IFSTRIP_ */

/*
 * Default number of supported streams. _NFILE is confusing and obsolete, but
 * supported anyway for backwards compatibility.
 */
#define _NFILE      _NSTREAM_

#ifdef _WIN32

#define _NSTREAM_   512

/*
 * Number of entries in _iob[] (declared below). Note that _NSTREAM_ must be
 * greater than or equal to _IOB_ENTRIES.
 */
#define _IOB_ENTRIES 20

#else  /* _WIN32 */

#ifdef CRTDLL
#define _NSTREAM_   128         /* *MUST* match the value under ifdef _DLL! */
#else  /* CRTDLL */
#ifdef _DLL
#define _NSTREAM_   128
#else  /* _DLL */
#ifdef _MT
#define _NSTREAM_   40
#else  /* _MT */
#define _NSTREAM_   20
#endif  /* _MT */
#endif  /* _DLL */
#endif  /* CRTDLL */

#endif  /* _WIN32 */

#define EOF     (-1)


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
#endif  /* _FILE_DEFINED */


#ifndef _MAC

/* Directory where temporary files may be created. */

#define _P_tmpdir   "\\"
#define _wP_tmpdir  L"\\"

/* L_tmpnam = size of P_tmpdir
 *            + 1 (in case P_tmpdir does not end in "/")
 *            + 12 (for the filename string)
 *            + 1 (for the null terminator)
 */
#define L_tmpnam sizeof(_P_tmpdir)+12

#else  /* _MAC */

#define L_tmpnam 255

#endif  /* _MAC */




/* Seek method constants */

#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0


#define FILENAME_MAX    260
#define FOPEN_MAX       20
#define _SYS_OPEN       20
#define TMP_MAX         32767


/* Define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */


/* Declare _iob[] array */

#ifndef _STDIO_DEFINED
#if defined (_DLL) && defined (_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP extern FILE * __cdecl __p__iob(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
_CRTIMP extern FILE _iob[];
#endif  /* _STDIO_DEFINED */


/* Define file position type */

#ifndef _FPOS_T_DEFINED
#undef _FPOSOFF

#if !__STDC__ && _INTEGRAL_MAX_BITS >= 64   
typedef __int64 fpos_t;
#define _FPOSOFF(fp) ((long)(fp))
#else  /* !__STDC__ && _INTEGRAL_MAX_BITS >= 64    */
typedef struct fpos_t {
        unsigned int lopart;
        int          hipart;
        } fpos_t;
#define _FPOSOFF(fp) ((long)(fp).lopart)
#endif  /* !__STDC__ && _INTEGRAL_MAX_BITS >= 64    */

#define _FPOS_T_DEFINED
#endif  /* _FPOS_T_DEFINED */


#define stdin  (&_iob[0])
#define stdout (&_iob[1])
#define stderr (&_iob[2])


#define _IOREAD         0x0001
#define _IOWRT          0x0002

#define _IOFBF          0x0000
#define _IOLBF          0x0040
#define _IONBF          0x0004

#define _IOMYBUF        0x0008
#define _IOEOF          0x0010
#define _IOERR          0x0020
#define _IOSTRG         0x0040
#define _IORW           0x0080


/* Function prototypes */

#ifndef _STDIO_DEFINED

_CRTIMP int __cdecl _filbuf(FILE *);
_CRTIMP int __cdecl _flsbuf(int, FILE *);

_CRTIMP FILE * __cdecl _fsopen(const char *, const char *, int);

_CRTIMP void __cdecl clearerr(FILE *);
_CRTIMP int __cdecl fclose(FILE *);
_CRTIMP int __cdecl _fcloseall(void);

_CRTIMP FILE * __cdecl _fdopen(int, const char *);

_CRTIMP int __cdecl feof(FILE *);
_CRTIMP int __cdecl ferror(FILE *);
_CRTIMP int __cdecl fflush(FILE *);
_CRTIMP int __cdecl fgetc(FILE *);
_CRTIMP int __cdecl _fgetchar(void);
_CRTIMP int __cdecl fgetpos(FILE *, fpos_t *);
_CRTIMP char * __cdecl fgets(char *, int, FILE *);

_CRTIMP int __cdecl _fileno(FILE *);

_CRTIMP int __cdecl _flushall(void);
_CRTIMP FILE * __cdecl fopen(const char *, const char *);
_CRTIMP int __cdecl fprintf(FILE *, const char *, ...);
_CRTIMP int __cdecl fputc(int, FILE *);
_CRTIMP int __cdecl _fputchar(int);
_CRTIMP int __cdecl fputs(const char *, FILE *);
_CRTIMP size_t __cdecl fread(void *, size_t, size_t, FILE *);
_CRTIMP FILE * __cdecl freopen(const char *, const char *, FILE *);
_CRTIMP int __cdecl fscanf(FILE *, const char *, ...);
_CRTIMP int __cdecl fsetpos(FILE *, const fpos_t *);
_CRTIMP int __cdecl fseek(FILE *, long, int);
_CRTIMP long __cdecl ftell(FILE *);
_CRTIMP size_t __cdecl fwrite(const void *, size_t, size_t, FILE *);
_CRTIMP int __cdecl getc(FILE *);
_CRTIMP int __cdecl getchar(void);
_CRTIMP int __cdecl _getmaxstdio(void);
_CRTIMP char * __cdecl gets(char *);
_CRTIMP int __cdecl _getw(FILE *);
_CRTIMP void __cdecl perror(const char *);
_CRTIMP int __cdecl _pclose(FILE *);
_CRTIMP FILE * __cdecl _popen(const char *, const char *);
_CRTIMP int __cdecl printf(const char *, ...);
_CRTIMP int __cdecl putc(int, FILE *);
_CRTIMP int __cdecl putchar(int);
_CRTIMP int __cdecl puts(const char *);
_CRTIMP int __cdecl _putw(int, FILE *);
_CRTIMP int __cdecl remove(const char *);
_CRTIMP int __cdecl rename(const char *, const char *);
_CRTIMP void __cdecl rewind(FILE *);
_CRTIMP int __cdecl _rmtmp(void);
_CRTIMP int __cdecl scanf(const char *, ...);
_CRTIMP void __cdecl setbuf(FILE *, char *);
_CRTIMP int __cdecl _setmaxstdio(int);
_CRTIMP int __cdecl setvbuf(FILE *, char *, int, size_t);
_CRTIMP int __cdecl _snprintf(char *, size_t, const char *, ...);
_CRTIMP int __cdecl sprintf(char *, const char *, ...);
_CRTIMP int __cdecl sscanf(const char *, const char *, ...);
_CRTIMP char * __cdecl _tempnam(const char *, const char *);
_CRTIMP FILE * __cdecl tmpfile(void);
_CRTIMP char * __cdecl tmpnam(char *);
_CRTIMP int __cdecl ungetc(int, FILE *);
_CRTIMP int __cdecl _unlink(const char *);
_CRTIMP int __cdecl vfprintf(FILE *, const char *, va_list);
_CRTIMP int __cdecl vprintf(const char *, va_list);
_CRTIMP int __cdecl _vsnprintf(char *, size_t, const char *, va_list);
_CRTIMP int __cdecl vsprintf(char *, const char *, va_list);

#ifndef _MAC
#ifndef _WSTDIO_DEFINED

/* wide function prototypes, also declared in wchar.h  */

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif  /* WEOF */

_CRTIMP FILE * __cdecl _wfsopen(const wchar_t *, const wchar_t *, int);

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

#define getwchar()              fgetwc(stdin)
#define putwchar(_c)            fputwc((_c),stdout)
#define getwc(_stm)             fgetwc(_stm)
#define putwc(_c,_stm)          fputwc(_c,_stm)

_CRTIMP FILE * __cdecl _wfdopen(int, const wchar_t *);
_CRTIMP FILE * __cdecl _wfopen(const wchar_t *, const wchar_t *);
_CRTIMP FILE * __cdecl _wfreopen(const wchar_t *, const wchar_t *, FILE *);
_CRTIMP void __cdecl _wperror(const wchar_t *);
_CRTIMP FILE * __cdecl _wpopen(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wremove(const wchar_t *);
_CRTIMP wchar_t * __cdecl _wtempnam(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl _wtmpnam(wchar_t *);

#ifdef _MT
wint_t __cdecl _getwc_lk(FILE *);                                           /* _MTHREAD_ONLY */
wint_t __cdecl _putwc_lk(wint_t, FILE *);                                   /* _MTHREAD_ONLY */
wint_t __cdecl _ungetwc_lk(wint_t, FILE *);                                 /* _MTHREAD_ONLY */
char * __cdecl _wtmpnam_lk(char *);                                         /* _MTHREAD_ONLY */
#else  /* _MT */
#define _getwc_lk(_stm)                         fgetwc(_stm)                /* _MTHREAD_ONLY */
#define _putwc_lk(_c,_stm)                      fputwc(_c,_stm)             /* _MTHREAD_ONLY */
#define _ungetwc_lk(_c,_stm)                    ungetwc(_c,_stm)            /* _MTHREAD_ONLY */
#define _wtmpnam_lk(_string)                    _wtmpnam(_string)           /* _MTHREAD_ONLY */
#endif  /* _MT */

#define _WSTDIO_DEFINED
#endif  /* _WSTDIO_DEFINED */
#endif  /* _MAC */

#define _STDIO_DEFINED
#endif  /* _STDIO_DEFINED */


/* Macro definitions */

#define feof(_stream)     ((_stream)->_flag & _IOEOF)
#define ferror(_stream)   ((_stream)->_flag & _IOERR)
#define _fileno(_stream)  ((_stream)->_file)
#define getc(_stream)     (--(_stream)->_cnt >= 0 \
                ? 0xff & *(_stream)->_ptr++ : _filbuf(_stream))
#define putc(_c,_stream)  (--(_stream)->_cnt >= 0 \
                ? 0xff & (*(_stream)->_ptr++ = (char)(_c)) :  _flsbuf((_c),(_stream)))
#define getchar()         getc(stdin)
#define putchar(_c)       putc((_c),stdout)

#define _getc_lk(_stream)       (--(_stream)->_cnt >= 0 ? 0xff & *(_stream)->_ptr++ : _filbuf(_stream))                                             /* _MTHREAD_ONLY */
#define _putc_lk(_c,_stream)    (--(_stream)->_cnt >= 0 ? 0xff & (*(_stream)->_ptr++ = (char)(_c)) :  _flsbuf((_c),(_stream)))  /* _MTHREAD_ONLY */
#define _getchar_lk()           _getc_lk(stdin)                                                                                                                                 /* _MTHREAD_ONLY */
#define _putchar_lk(_c)         _putc_lk((_c),stdout)                                                                                                                       /* _MTHREAD_ONLY */
#ifndef _MAC
#define _getwchar_lk()          _getwc_lk(stdin)                                                                                                                                    /* _MTHREAD_ONLY */
#define _putwchar_lk(_c)        _putwc_lk((_c),stdout)                                                                                                                      /* _MTHREAD_ONLY */
#endif  /* _MAC */


#ifdef _MT
#undef  getc
#undef  putc
#undef  getchar
#undef  putchar
#endif  /* _MT */

#ifdef _MT
int __cdecl _fclose_lk(FILE *);                                             /* _MTHREAD_ONLY */
int __cdecl _fflush_lk(FILE *);                                             /* _MTHREAD_ONLY */
size_t __cdecl _fread_lk(void *, size_t, size_t, FILE *);                   /* _MTHREAD_ONLY */
int __cdecl _fseek_lk(FILE *, long, int);                                   /* _MTHREAD_ONLY */
long __cdecl _ftell_lk(FILE *);                                             /* _MTHREAD_ONLY */
size_t __cdecl _fwrite_lk(const void *, size_t, size_t, FILE *);            /* _MTHREAD_ONLY */
char * __cdecl _tmpnam_lk(char *);                                          /* _MTHREAD_ONLY */
int __cdecl _ungetc_lk(int, FILE *);                                        /* _MTHREAD_ONLY */
#else  /* _MT */
#define _fclose_lk(_stm)                        fclose(_stm)                /* _MTHREAD_ONLY */
#define _fflush_lk(_stm)                        fflush(_stm)                /* _MTHREAD_ONLY */
#define _fread_lk(_buf,_siz,_cnt,_stm)          fread(_buf,_siz,_cnt,_stm)  /* _MTHREAD_ONLY */
#define _fseek_lk(_stm,_offset,_origin)         fseek(_stm,_offset,_origin) /* _MTHREAD_ONLY */
#define _ftell_lk(_stm)                         ftell(_stm)                 /* _MTHREAD_ONLY */
#define _fwrite_lk(_buf,_siz,_cnt,_stm)         fwrite(_buf,_siz,_cnt,_stm) /* _MTHREAD_ONLY */
#define _tmpnam_lk(_string)                     tmpnam(_string)             /* _MTHREAD_ONLY */
#define _ungetc_lk(_c,_stm)                     ungetc(_c,_stm)             /* _MTHREAD_ONLY */
#endif  /* _MT */


#if !__STDC__

/* Non-ANSI names for compatibility */

#define P_tmpdir  _P_tmpdir
#define SYS_OPEN  _SYS_OPEN

_CRTIMP int __cdecl fcloseall(void);
_CRTIMP FILE * __cdecl fdopen(int, const char *);
_CRTIMP int __cdecl fgetchar(void);
_CRTIMP int __cdecl fileno(FILE *);
_CRTIMP int __cdecl flushall(void);
_CRTIMP int __cdecl fputchar(int);
_CRTIMP int __cdecl getw(FILE *);
_CRTIMP int __cdecl putw(int, FILE *);
_CRTIMP int __cdecl rmtmp(void);
_CRTIMP char * __cdecl tempnam(const char *, const char *);
_CRTIMP int __cdecl unlink(const char *);

#endif  /* !__STDC__ */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifdef _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_STDIO */
#else /* UNIX */ 
// Load std Unix version
#include "/usr/include/stdio.h" 
#endif /* UNIX */ 
