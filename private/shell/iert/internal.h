/***
*internal.h - contains declarations of internal routines and variables
*
*       Copyright (c) 1985-1996, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Declares routines and variables used internally by the C run-time.
*
*       [Internal]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_INTERNAL
#define _INC_INTERNAL

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <cruntime.h>

/*
 * Conditionally include windows.h to pick up the definition of
 * CRITICAL_SECTION.
 */
#if defined (_MT)
#include <windows.h>
#endif  /* defined (_MT) */

/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#define _CRTAPI1
#endif  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#endif  /* _CRTAPI1 */


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#define _CRTAPI2
#endif  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#endif  /* _CRTAPI2 */


/* Define _CRTIMP */

#ifndef _CRTIMP
/* current definition */
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


/* Define _CRTIMP1 */

#ifndef _CRTIMP1
#ifdef CRTDLL1
#define _CRTIMP1 __declspec(dllexport)
#else  /* CRTDLL1 */
#define _CRTIMP1 _CRTIMP
#endif  /* CRTDLL1 */
#endif  /* _CRTIMP1 */


/* Define _CRTIMP2 */
#ifndef _CRTIMP2
#ifdef CRTDLL2
#define _CRTIMP2 __declspec(dllexport)
#else  /* CRTDLL2 */
#ifdef _DLL
#define _CRTIMP2 __declspec(dllimport)
#else  /* _DLL */
#define _CRTIMP2
#endif  /* _DLL */
#endif  /* CRTDLL2 */
#endif  /* _CRTIMP2 */


/* Define __cdecl for non-Microsoft compilers */

#if (!defined (_MSC_VER) && !defined (__cdecl))
#define __cdecl
#endif  /* (!defined (_MSC_VER) && !defined (__cdecl)) */

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif  /* _WCHAR_T_DEFINED */

/* Define function type used in several startup sources */

typedef void (__cdecl *_PVFV)(void);



#if defined (_DLL) && defined (_M_IX86)
#ifndef SPECIAL_CRTEXE
#define _commode    (*__p__commode())
#endif  /* SPECIAL_CRTEXE */
_CRTIMP int * __p__commode(void);
#else  /* defined (_DLL) && defined (_M_IX86) */
#if defined (SPECIAL_CRTEXE) && defined (_DLL)
        extern int _commode;
#else  /* defined (SPECIAL_CRTEXE) && defined (_DLL) */
_CRTIMP extern int _commode;
#endif  /* defined (SPECIAL_CRTEXE) && defined (_DLL) */
#endif  /* defined (_DLL) && defined (_M_IX86) */


#ifdef _WIN32

/*
 * Control structure for lowio file handles
 */
typedef struct {
        long osfhnd;    /* underlying OS file HANDLE */
        char osfile;    /* attributes of file (e.g., open in text mode?) */
        char pipech;    /* one char buffer for handles opened on pipes */
#if defined (_MT)
        int lockinitflag;
        CRITICAL_SECTION lock;
#endif  /* defined (_MT) */
    }   ioinfo;

/*
 * Definition of IOINFO_L2E, the log base 2 of the number of elements in each
 * array of ioinfo structs.
 */
#define IOINFO_L2E          5

/*
 * Definition of IOINFO_ARRAY_ELTS, the number of elements in ioinfo array
 */
#define IOINFO_ARRAY_ELTS   (1 << IOINFO_L2E)

/*
 * Definition of IOINFO_ARRAYS, maximum number of supported ioinfo arrays.
 */
#define IOINFO_ARRAYS       64

#define _NHANDLE_           (IOINFO_ARRAYS * IOINFO_ARRAY_ELTS)

/*
 * Access macros for getting at an ioinfo struct and its fields from a
 * file handle
 */
#define _pioinfo(i) ( __pioinfo[i >> IOINFO_L2E] + (i & (IOINFO_ARRAY_ELTS - \
                              1)) )
#define _osfhnd(i)  ( _pioinfo(i)->osfhnd )

#define _osfile(i)  ( _pioinfo(i)->osfile )

#define _pipech(i)  ( _pioinfo(i)->pipech )

/*
 * Safer versions of the above macros. Currently, only _osfile_safe is
 * used.
 */
#define _pioinfo_safe(i)    ( (i != -1) ? _pioinfo(i) : &__badioinfo )

#define _osfhnd_safe(i)     ( _pioinfo_safe(i)->osfhnd )

#define _osfile_safe(i)     ( _pioinfo_safe(i)->osfile )

#define _pipech_safe(i)     ( _pioinfo_safe(i)->pipech )

/*
 * Special, static ioinfo structure used only for more graceful handling
 * of a C file handle value of -1 (results from common errors at the stdio
 * level).
 */
extern _CRTIMP ioinfo __badioinfo;

/*
 * Array of arrays of control structures for lowio files.
 */
extern _CRTIMP ioinfo * __pioinfo[];

/*
 * Current number of allocated ioinfo structures (_NHANDLE_ is the upper
 * limit).
 */
extern int _nhandle;

#else  /* _WIN32 */

/*
 * Define the number of supported handles. This definition must exactly match
 * the one in mtdll.h.
 */
#ifdef CRTDLL
#define _NHANDLE_   512     /* *MUST* match the value under ifdef _DLL! */
#else  /* CRTDLL */
#ifdef _DLL
#define _NHANDLE_   512
#else  /* _DLL */
#ifdef _MT
#define _NHANDLE_   256
#else  /* _MT */
#define _NHANDLE_   64
#endif  /* _MT */
#endif  /* _DLL */
#endif  /* CRTDLL */

extern int _nhandle;        /* == _NHANDLE_, set in ioinit.c */

extern char _osfile[];

extern  int _osfhnd[];

#if defined (_M_M68K) || defined (_M_MPPC)
extern unsigned char _osperm[];
extern short _osVRefNum[];
extern int _nfile;                /*old -- check sources */
extern unsigned int _tmpoff;      /*old -- check source */

extern unsigned char _osfileflags[];
#define FTEMP           0x01

#endif  /* defined (_M_M68K) || defined (_M_MPPC) */

#endif  /* _WIN32 */

int __cdecl _alloc_osfhnd(void);
int __cdecl _free_osfhnd(int);
int __cdecl _set_osfhnd(int,long);


extern const char __dnames[];
extern const char __mnames[];

extern int _days[];
extern int _lpdays[];

#ifndef _TIME_T_DEFINED
typedef long time_t;        /* time value */
#define _TIME_T_DEFINED     /* avoid multiple def's of time_t */
#endif  /* _TIME_T_DEFINED */

#if defined (_M_M68K) || defined (_M_MPPC)
extern time_t __cdecl  _gmtotime_t (int, int, int, int, int, int);
#endif  /* defined (_M_M68K) || defined (_M_MPPC) */

extern time_t __cdecl __loctotime_t(int, int, int, int, int, int, int);

#ifdef _TM_DEFINED
extern int __cdecl _isindst(struct tm *);
#endif  /* _TM_DEFINED */

extern void __cdecl __tzset(void);

extern int __cdecl _validdrive(unsigned);


/**
** This variable is in the C start-up; the length must be kept synchronized
**  It is used by the *cenvarg.c modules
**/

extern char _acfinfo[]; /* "_C_FILE_INFO=" */

#define CFI_LENGTH  12  /* "_C_FILE_INFO" is 12 bytes long */


/* typedefs needed for subsequent prototypes */

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif  /* _SIZE_T_DEFINED */

#ifndef _VA_LIST_DEFINED
#ifdef _M_ALPHA
typedef struct {
        char *a0;   /* pointer to first homed integer argument */
        int offset; /* byte offset of next parameter */
} va_list;
#else  /* _M_ALPHA */
typedef char *  va_list;
#endif  /* _M_ALPHA */
#define _VA_LIST_DEFINED
#endif  /* _VA_LIST_DEFINED */

/*
 * stdio internals
 */
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

#if !defined (_M_MPPC) && !defined (_M_M68K)

#if !defined (_FILEX_DEFINED) && defined (_WINDOWS_)

/*
 * Variation of FILE type used for the dynamically allocated portion of
 * __piob[]. For single thread, _FILEX is the same as FILE. For multithread
 * models, _FILEX has two fields: the FILE struct and the CRITICAL_SECTION
 * struct used to serialize access to the FILE.
 */
#if defined (_MT)

typedef struct {
        FILE f;
        CRITICAL_SECTION lock;
        }   _FILEX;

#else  /* defined (_MT) */

typedef FILE    _FILEX;

#endif  /* defined (_MT) */

#define _FILEX_DEFINED
#endif  /* !defined (_FILEX_DEFINED) && defined (_WINDOWS_) */


/*
 * Number of entries supported in the array pointed to by __piob[]. That is,
 * the number of stdio-level files which may be open simultaneously. This
 * is normally set to _NSTREAM_ by the stdio initialization code.
 */
extern int _nstream;

/*
 * Pointer to the array of pointers to FILE/_FILEX structures that are used
 * to manage stdio-level files.
 */
extern void **__piob;


#endif  /* !defined (_M_MPPC) && !defined (_M_M68K) */

#if defined (_M_MPPC) || defined (_M_M68K)
extern FILE * _lastiob;
#endif  /* defined (_M_MPPC) || defined (_M_M68K) */

FILE * __cdecl _getstream(void);
FILE * __cdecl _openfile(const char *, const char *, int, FILE *);
#ifdef _WIN32
FILE * __cdecl _wopenfile(const wchar_t *, const wchar_t *, int, FILE *);
#endif  /* _WIN32 */
void __cdecl _getbuf(FILE *);
int __cdecl _filwbuf (FILE *);
int __cdecl _flswbuf(int, FILE *);
void __cdecl _freebuf(FILE *);
int __cdecl _stbuf(FILE *);
void __cdecl _ftbuf(int, FILE *);
int __cdecl _output(FILE *, const char *, va_list);
#ifdef _WIN32
int __cdecl _woutput(FILE *, const wchar_t *, va_list);
#endif  /* _WIN32 */
int __cdecl _input(FILE *, const unsigned char *, va_list);
#ifdef _WIN32
int __cdecl _winput(FILE *, const wchar_t *, va_list);
#endif  /* _WIN32 */
int __cdecl _flush(FILE *);
void __cdecl _endstdio(void);

#ifdef _WIN32
int __cdecl _fseeki64(FILE *, __int64, int);
int __cdecl _fseeki64_lk(FILE *, __int64, int);
__int64 __cdecl _ftelli64(FILE *);
#ifdef _MT
__int64 __cdecl _ftelli64_lk(FILE *);
#else  /* _MT */
#define _ftelli64_lk    _ftelli64
#endif  /* _MT */
#endif  /* _WIN32 */

#ifndef CRTDLL
extern int _cflush;
#endif  /* CRTDLL */


extern unsigned int _tempoff;

extern unsigned int _old_pfxlen;

extern int _umaskval;       /* the umask value */

extern char _pipech[];      /* pipe lookahead */

extern char _exitflag;      /* callable termination flag */

extern int _C_Termination_Done; /* termination done flag */


char * __cdecl _getpath(const char *, char *, unsigned);
#ifdef _WIN32
wchar_t * __cdecl _wgetpath(const wchar_t *, wchar_t *, unsigned);
#endif  /* _WIN32 */

extern int _dowildcard;     /* flag to enable argv[] wildcard expansion */

#ifndef _PNH_DEFINED
typedef int (__cdecl * _PNH)( size_t );
#define _PNH_DEFINED
#endif  /* _PNH_DEFINED */


/* calls the currently installed new handler */
int _callnewh(size_t);

extern int _newmode;    /* malloc new() handler mode */

#if defined (_DLL) && defined (_M_IX86)

/* pointer to initial environment block that is passed to [w]main */
#define __winitenv  (*__p___winitenv())
_CRTIMP wchar_t *** __cdecl __p___winitenv(void);
#define __initenv  (*__p___initenv())
_CRTIMP char *** __cdecl __p___initenv(void);

#else  /* defined (_DLL) && defined (_M_IX86) */

/* pointer to initial environment block that is passed to [w]main */
#ifdef _WIN32
extern _CRTIMP wchar_t **__winitenv;
#endif  /* _WIN32 */
extern _CRTIMP char **__initenv;

#endif  /* defined (_DLL) && defined (_M_IX86) */


/* startup set values */
extern char *_aenvptr;      /* environment ptr */
#ifdef _WIN32
extern wchar_t *_wenvptr;   /* wide environment ptr */
#endif  /* _WIN32 */


/* command line */


#if defined (_DLL) && defined (_M_IX86)
#define _acmdln     (*__p__acmdln())
_CRTIMP char ** __cdecl __p__acmdln(void);
#define _wcmdln     (*__p__wcmdln())
_CRTIMP wchar_t ** __cdecl __p__wcmdln(void);
#else  /* defined (_DLL) && defined (_M_IX86) */
_CRTIMP extern char *_acmdln;
#ifdef _WIN32
_CRTIMP extern wchar_t *_wcmdln;
#endif  /* _WIN32 */
#endif  /* defined (_DLL) && defined (_M_IX86) */



/*
 * prototypes for internal startup functions
 */
int __cdecl _cwild(void);           /* wild.c */
#ifdef _WIN32
int __cdecl _wcwild(void);          /* wwild.c */
#endif  /* _WIN32 */
#ifdef _MT
int __cdecl _mtinit(void);          /* tidtable.asm */
void __cdecl _mtterm(void);         /* tidtable.asm */
void __cdecl _mtinitlocks(void);        /* mlock.asm */
void __cdecl _mtdeletelocks(void);      /* mlock.asm */
#endif  /* _MT */

/*
 * C source build only!!!!
 *
 * more prototypes for internal startup functions
 */
void __cdecl _amsg_exit(int);           /* crt0.c */
#if defined (_M_M68K) || defined (_M_MPPC)
int __cdecl __cinit(void);              /* crt0dat.c */
#else  /* defined (_M_M68K) || defined (_M_MPPC) */
void __cdecl _cinit(void);              /* crt0dat.c */
#endif  /* defined (_M_M68K) || defined (_M_MPPC) */
void __cdecl __doinits(void);           /* astart.asm */
void __cdecl __doterms(void);           /* astart.asm */
void __cdecl __dopreterms(void);        /* astart.asm */
void __cdecl _FF_MSGBANNER(void);
void __cdecl _fptrap(void);             /* crt0fp.c */
#if defined (_M_M68K) || defined (_M_MPPC)
void __cdecl _heap_init(void);
#else  /* defined (_M_M68K) || defined (_M_MPPC) */
int __cdecl _heap_init(void);
#endif  /* defined (_M_M68K) || defined (_M_MPPC) */
void __cdecl _heap_term(void);
void __cdecl _heap_abort(void);
#ifdef _WIN32
void __cdecl __initconin(void);         /* initcon.c */
void __cdecl __initconout(void);        /* initcon.c */
#endif  /* _WIN32 */
void __cdecl _ioinit(void);             /* crt0.c, crtlib.c */
void __cdecl _ioterm(void);             /* crt0.c, crtlib.c */
char * __cdecl _GET_RTERRMSG(int);
void __cdecl _NMSG_WRITE(int);
void __cdecl _setargv(void);            /* setargv.c, stdargv.c */
void __cdecl __setargv(void);           /* stdargv.c */
#ifdef _WIN32
void __cdecl _wsetargv(void);           /* wsetargv.c, wstdargv.c */
void __cdecl __wsetargv(void);          /* wstdargv.c */
#endif  /* _WIN32 */
void __cdecl _setenvp(void);            /* stdenvp.c */
#ifdef _WIN32
void __cdecl _wsetenvp(void);           /* wstdenvp.c */
#endif  /* _WIN32 */
void __cdecl __setmbctable(unsigned int);   /* mbctype.c */

#if defined (_M_M68K) || defined (_M_MPPC)
void __cdecl _envinit(void);            /* intcon.c */
int __cdecl __dupx(int, int);           /* dupx.c */
#define SystemSevenOrLater  1
#include "types.h"
void _ShellReturn(void);                /* astart.a */
extern int _shellStack;                 /* astart.a */
int __GestaltAvailable(void);           /* gestalt.c */
int __TrapFromGestalt(OSType selector, long bitNum); /* gestalt.c */
int SzPathNameFromDirID(long lDirID, char * szPath, int cbLen);
void __cdecl _endlowio(void);           /* endlow.c */
void __cdecl _initcon(void);            /* intcon.c */
void __cdecl _inittime(void);           /* clock.c */
void __cdecl _onexitinit (void);        /* onexit.c */
#endif  /* defined (_M_M68K) || defined (_M_MPPC) */

#if defined (_MBCS)
void __cdecl __initmbctable(void);      /* mbctype.c */
#endif  /* defined (_MBCS) */

int __cdecl main(int, char **, char **);
#ifdef _WIN32
int __cdecl wmain(int, wchar_t **, wchar_t **);
#endif  /* _WIN32 */

#ifdef _WIN32
/* helper functions for wide/multibyte environment conversion */
int __cdecl __mbtow_environ (void);
int __cdecl __wtomb_environ (void);
int __cdecl __crtsetenv (const char *, const int);
int __cdecl __crtwsetenv (const wchar_t *, const int);
#endif  /* _WIN32 */


_CRTIMP extern void (__cdecl * _aexit_rtn)(int);


#if defined (_DLL) || defined (CRTDLL)

#ifndef _STARTUP_INFO_DEFINED
typedef struct
{
        int newmode;
} _startupinfo;
#define _STARTUP_INFO_DEFINED
#endif  /* _STARTUP_INFO_DEFINED */

_CRTIMP void __cdecl __getmainargs(int *, char ***, char ***, int, _startupinfo *);

#ifdef _WIN32
_CRTIMP void __cdecl __wgetmainargs(int *, wchar_t ***, wchar_t ***, int, _startupinfo *);
#endif  /* _WIN32 */

#endif  /* defined (_DLL) || defined (CRTDLL) */

/*
 * Prototype, variables and constants which determine how error messages are
 * written out.
 */
#define _UNKNOWN_APP    0
#define _CONSOLE_APP    1
#define _GUI_APP        2

extern int __app_type;

extern int __error_mode;

_CRTIMP void __cdecl __set_app_type(int);

/*
 * C source build only!!!!
 *
 * map Win32 errors into Xenix errno values -- for modules written in C
 */
#ifdef _WIN32
extern void __cdecl _dosmaperr(unsigned long);
#else  /* _WIN32 */
extern void __cdecl _dosmaperr(short);
#endif  /* _WIN32 */

/*
 * internal routines used by the exec/spawn functions
 */

extern int __cdecl _dospawn(int, const char *, char *, char *);
#ifdef _WIN32
extern int __cdecl _wdospawn(int, const wchar_t *, wchar_t *, wchar_t *);
#endif  /* _WIN32 */
extern int __cdecl _cenvarg(const char * const *, const char * const *,
        char **, char **, const char *);
#ifdef _WIN32
extern int __cdecl _wcenvarg(const wchar_t * const *, const wchar_t * const *,
        wchar_t **, wchar_t **, const wchar_t *);
#endif  /* _WIN32 */
#ifndef _M_IX86
extern char ** _capture_argv(va_list *, const char *, char **, size_t);
#ifdef _WIN32
extern wchar_t ** _wcapture_argv(va_list *, const wchar_t *, wchar_t **, size_t);
#endif  /* _WIN32 */
#endif  /* _M_IX86 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif  /* _INC_INTERNAL */
