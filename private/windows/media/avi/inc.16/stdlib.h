/***
*stdlib.h - declarations/definitions for commonly used library functions
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This include file contains the function declarations for
*   commonly used library functions which either don't fit somewhere
*   else, or, like toupper/tolower, can't be declared in the normal
*   place for other reasons.
*   [ANSI]
*
****/

#ifndef _INC_STDLIB

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#define __near      _near
#define __pascal    _pascal
#endif 

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif 

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif 

/* define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else 
#define NULL    ((void *)0)
#endif 
#endif 

/* exit() arg values */

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1

#ifndef _ONEXIT_T_DEFINED
typedef int (__cdecl * _onexit_t)();
typedef int (__far __cdecl * _fonexit_t)();
#ifndef __STDC__
/* Non-ANSI name for compatibility */
typedef int (__cdecl * onexit_t)();
#endif 
#define _ONEXIT_T_DEFINED
#endif 


/* data structure definitions for div and ldiv runtimes. */

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

/* maximum value that can be returned by the rand function. */

#define RAND_MAX 0x7fff

extern unsigned short __mb_cur_max; /* mb-len for curr. locale */
#define MB_CUR_MAX __mb_cur_max


/* min and max macros */

#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))


/* sizes for buffers used by the _makepath() and _splitpath() functions.
 * note that the sizes include space for 0-terminator
 */

#define _MAX_PATH   260 /* max. length of full pathname */
#define _MAX_DRIVE  3   /* max. length of drive component */
#define _MAX_DIR    256 /* max. length of path component */
#define _MAX_FNAME  256 /* max. length of file name component */
#define _MAX_EXT    256 /* max. length of extension component */

/* external variable declarations */

#ifdef _MT
extern int __far * __cdecl __far volatile _errno(void);
extern int __far * __cdecl __far __doserrno(void);
#define errno       (*_errno())
#define _doserrno   (*__doserrno())
#else 
extern int __near __cdecl volatile errno;   /* error value */
extern int __near __cdecl _doserrno;        /* OS system error value */
#endif 

extern char * __near __cdecl _sys_errlist[];    /* perror error message table */
extern int __near __cdecl _sys_nerr;        /* # of entries in sys_errlist table */
extern char ** __near __cdecl _environ;     /* pointer to environment table */
extern int __near __cdecl _fmode;       /* default file translation mode */
#ifndef _WINDOWS
extern int __near __cdecl _fileinfo;        /* open file info mode (for spawn) */
#endif 

extern unsigned int __near __cdecl _psp;    /* Program Segment Prefix */

extern char __far * __near __cdecl _pgmptr; /* Pointer to Program name */

/* DOS and Windows major/minor version numbers */

extern unsigned int __near __cdecl _osver;
extern unsigned char __near __cdecl _osmajor;
extern unsigned char __near __cdecl _osminor;
extern unsigned int __near __cdecl _winver;
extern unsigned char __near __cdecl _winmajor;
extern unsigned char __near __cdecl _winminor;

/* OS mode */

#define _DOS_MODE   0   /* DOS */
#define _OS2_MODE   1   /* OS/2 */
#define _WIN_MODE   2   /* Windows */

extern unsigned char __near __cdecl _osmode;

/* CPU mode */

#define _REAL_MODE  0   /* real mode */
#define _PROT_MODE  1   /* protect mode */

extern unsigned char __near __cdecl _cpumode;

/* function prototypes */

#ifdef _MT
double __pascal atof(const char *);
double __pascal strtod(const char *, char * *);
ldiv_t __pascal ldiv(long, long);
#else 
double __cdecl atof(const char *);
double __cdecl strtod(const char *, char * *);
ldiv_t __cdecl ldiv(long, long);
#endif 

void __cdecl abort(void);
int __cdecl abs(int);
int __cdecl atexit(void (__cdecl *)(void));
int __cdecl atoi(const char *);
long __cdecl atol(const char *);
long double __cdecl _atold(const char *);
void * __cdecl bsearch(const void *, const void *,
    size_t, size_t, int (__cdecl *)(const void *,
    const void *));
void * __cdecl calloc(size_t, size_t);
div_t __cdecl div(int, int);
char * __cdecl _ecvt(double, int, int *, int *);
#ifndef _WINDLL
void __cdecl exit(int);
void __cdecl _exit(int);
#endif 
int __far __cdecl _fatexit(void (__cdecl __far *)(void));
char * __cdecl _fcvt(double, int, int *, int *);
_fonexit_t __far __cdecl _fonexit(_fonexit_t);
void __cdecl free(void *);
char * __cdecl _fullpath(char *, const char *,
    size_t);
char * __cdecl _gcvt(double, int, char *);
char * __cdecl getenv(const char *);
char * __cdecl _itoa(int, char *, int);
long __cdecl labs(long);
unsigned long __cdecl _lrotl(unsigned long, int);
unsigned long __cdecl _lrotr(unsigned long, int);
char * __cdecl _ltoa(long, char *, int);
void __cdecl _makepath(char *, const char *,
    const char *, const char *, const char *);
void * __cdecl malloc(size_t);
_onexit_t __cdecl _onexit(_onexit_t);
#ifndef _WINDLL
void __cdecl perror(const char *);
#endif 
int __cdecl _putenv(const char *);
void __cdecl qsort(void *, size_t, size_t, int (__cdecl *)
    (const void *, const void *));
unsigned int __cdecl _rotl(unsigned int, int);
unsigned int __cdecl _rotr(unsigned int, int);
int __cdecl rand(void);
void * __cdecl realloc(void *, size_t);
void __cdecl _searchenv(const char *, const char *,
    char *);
void __cdecl _splitpath(const char *, char *,
    char *, char *, char *);
void __cdecl srand(unsigned int);
long __cdecl strtol(const char *, char * *,
    int);
long double __cdecl _strtold(const char *,
    char * *);
unsigned long __cdecl strtoul(const char *,
    char * *, int);
void __cdecl _swab(char *, char *, int);
#ifndef _WINDOWS
int __cdecl system(const char *);
#endif 
char * __cdecl _ultoa(unsigned long, char *, int);

int __cdecl mblen(const char *, size_t);
int __cdecl mbtowc(wchar_t *, const char *, size_t);
int __cdecl wctomb(char *, wchar_t);
size_t __cdecl mbstowcs(wchar_t *, const char *, size_t);
size_t __cdecl wcstombs(char *, const wchar_t *, size_t);

/* model-independent function prototypes */

int __far __cdecl _fmblen(const char __far *, size_t);
int __far __cdecl _fmbtowc(wchar_t __far *, const char __far *,
    size_t);
int __far __cdecl _fwctomb(char __far *, wchar_t);
size_t __far __cdecl _fmbstowcs(wchar_t __far *, const char __far *,
    size_t);
size_t __far __cdecl _fwcstombs(char __far *, const wchar_t __far *,
    size_t);

#ifndef tolower
int __cdecl tolower(int);
#endif 

#ifndef toupper
int __cdecl toupper(int);
#endif 

#ifndef __STDC__
/* Non-ANSI names for compatibility */

#ifndef __cplusplus
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif 

extern char * __near __cdecl sys_errlist[];
extern int __near __cdecl sys_nerr;
extern char ** __near __cdecl environ;

#define DOS_MODE    _DOS_MODE
#define OS2_MODE    _OS2_MODE

char * __cdecl ecvt(double, int, int *, int *);
char * __cdecl fcvt(double, int, int *, int *);
char * __cdecl gcvt(double, int, char *);
char * __cdecl itoa(int, char *, int);
char * __cdecl ltoa(long, char *, int);
onexit_t __cdecl onexit(onexit_t);
int __cdecl putenv(const char *);
void __cdecl swab(char *, char *, int);
char * __cdecl ultoa(unsigned long, char *, int);

#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_STDLIB
#endif 
