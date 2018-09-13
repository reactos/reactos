/***
*mtdll.h - DLL/Multi-thread include
*
* Copyright (c) 1987 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*
*       [Internal]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_MTDLL
#define _INC_MTDLL

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


/*
 * Define the number of supported handles and streams. The definitions
 * here must exactly match those in internal.h (for _NHANDLE_) and stdio.h
 * (for _NSTREAM_).
 */

#ifdef _WIN32

#define _IOB_ENTRIES    20

#else  /* _WIN32 */

/*
 * Define the number of supported handles and streams. The definitions
 * here must exactly match those in internal.h (for _NHANDLE_) and stdio.h
 * (for _NSTREAM_).
 */

#ifdef CRTDLL
#define _NHANDLE_   512     /* *MUST* match the value under ifdef _DLL! */
#define _NSTREAM_   128     /* *MUST* match the value under ifdef _DLL! */
#else  /* CRTDLL */
#ifdef _DLL
#define _NHANDLE_   512
#define _NSTREAM_   128
#else  /* _DLL */
#ifdef _MT
#define _NHANDLE_   256
#define _NSTREAM_   40
#else  /* _MT */
#define _NHANDLE_   64
#define _NSTREAM_   20
#endif  /* _MT */
#endif  /* _DLL */
#endif  /* CRTDLL */

#endif  /* _WIN32 */

/* Lock symbols */

/* ---- do not change lock #1 without changing emulator ---- */
#define _SIGNAL_LOCK    1       /* lock for signal() & emulator SignalAddress */
                                /* emulator uses \math\include\mtdll.inc     */

#define _IOB_SCAN_LOCK  2       /* _iob[] table lock                */
#define _TMPNAM_LOCK    3       /* lock global tempnam variables    */
#define _INPUT_LOCK     4       /* lock for _input() routine        */
#define _OUTPUT_LOCK    5       /* lock for _output() routine       */
#define _CSCANF_LOCK    6       /* lock for _cscanf() routine       */
#define _CPRINTF_LOCK   7       /* lock for _cprintf() routine      */
#define _CONIO_LOCK     8       /* lock for conio routines          */
#define _HEAP_LOCK      9       /* lock for heap allocator routines */
/* #define _BHEAP_LOCK  10         Obsolete                         */
#define _TIME_LOCK      11      /* lock for time functions          */
#define _ENV_LOCK       12      /* lock for environment variables   */
#define _EXIT_LOCK1     13      /* lock #1 for exit code            */
/* #define _EXIT_LOCK2  14         Obsolete                         */
/* #define _THREADDATA_LOCK 15     Obsolete                         */
#define _POPEN_LOCK     16      /* lock for _popen/_pclose database */
#define _LOCKTAB_LOCK   17      /* lock to protect semaphore lock table */
#define _OSFHND_LOCK    18      /* lock to protect _osfhnd array    */
#define _SETLOCALE_LOCK 19      /* lock for locale handles, etc.    */
/* #define _LC_COLLATE_LOCK 20     Obsolete                         */
/* #define _LC_CTYPE_LOCK  21      Obsolete                         */
/* #define _LC_MONETARY_LOCK 22    Obsolete                         */
/* #define _LC_NUMERIC_LOCK 23     Obsolete                         */
/* #define _LC_TIME_LOCK   24      Obsolete                         */
#define _MB_CP_LOCK     25      /* lock for multibyte code page     */
#define _NLG_LOCK       26      /* lock for NLG notifications       */
#define _TYPEINFO_LOCK  27      /* lock for type_info access        */

#define _STREAM_LOCKS   28      /* Table of stream locks            */

#ifdef _WIN32
#define _LAST_STREAM_LOCK  (_STREAM_LOCKS+_IOB_ENTRIES-1)   /* Last stream lock */
#else  /* _WIN32 */
#define _LAST_STREAM_LOCK  (_STREAM_LOCKS+_NSTREAM_-1)  /* Last stream lock */
#endif  /* _WIN32 */


#ifdef _WIN32

#define _TOTAL_LOCKS        (_LAST_STREAM_LOCK+1)

#else  /* _WIN32 */

#define _FH_LOCKS           (_LAST_STREAM_LOCK+1)   /* Table of fh locks */

#define _LAST_FH_LOCK       (_FH_LOCKS+_NHANDLE_-1) /* Last fh lock      */

#define _TOTAL_LOCKS        (_LAST_FH_LOCK+1)       /* Total number of locks */

#endif  /* _WIN32 */


#define _LOCK_BIT_INTS     (_TOTAL_LOCKS/(sizeof(unsigned)*8))+1   /* # of ints to hold lock bits */

#ifndef __assembler

/* Multi-thread macros and prototypes */

#if defined (_MT)

#ifndef UNIX
#ifdef _WIN32
/* need wchar_t for _wtoken field in _tiddata */
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif  /* _WCHAR_T_DEFINED */
#endif  /* _WIN32 */
#endif // UNIX

_CRTIMP extern unsigned long __cdecl __threadid(void);
#define _threadid   (__threadid())
_CRTIMP extern unsigned long __cdecl __threadhandle(void);
#define _threadhandle   (__threadhandle())



/* Structure for each thread's data */

struct _tiddata {
        unsigned long   _tid;       /* thread ID */


        unsigned long   _thandle;   /* thread handle */

        int     _terrno;            /* errno value */
        unsigned long   _tdoserrno; /* _doserrno value */
        unsigned int    _fpds;      /* Floating Point data segment */
        unsigned long   _holdrand;  /* rand() seed value */
        char *      _token;         /* ptr to strtok() token */
#ifdef _WIN32
        wchar_t *   _wtoken;        /* ptr to wcstok() token */
#endif  /* _WIN32 */
        unsigned char * _mtoken;    /* ptr to _mbstok() token */

        /* following pointers get malloc'd at runtime */
        char *      _errmsg;        /* ptr to strerror()/_strerror() buff */
        char *      _namebuf0;      /* ptr to tmpnam() buffer */
#ifdef _WIN32
        wchar_t *   _wnamebuf0;     /* ptr to _wtmpnam() buffer */
#endif  /* _WIN32 */
        char *      _namebuf1;      /* ptr to tmpfile() buffer */
#ifdef _WIN32
        wchar_t *   _wnamebuf1;     /* ptr to _wtmpfile() buffer */
#endif  /* _WIN32 */
        char *      _asctimebuf;    /* ptr to asctime() buffer */
#ifdef _WIN32
        wchar_t *   _wasctimebuf;   /* ptr to _wasctime() buffer */
#endif  /* _WIN32 */
        void *      _gmtimebuf;     /* ptr to gmtime() structure */
        char *      _cvtbuf;        /* ptr to ecvt()/fcvt buffer */

        /* following fields are needed by _beginthread code */
        void *      _initaddr;      /* initial user thread address */
        void *      _initarg;       /* initial user thread argument */

        /* following three fields are needed to support signal handling and
         * runtime errors */
        void *      _pxcptacttab;   /* ptr to exception-action table */
        void *      _tpxcptinfoptrs; /* ptr to exception info pointers */
        int         _tfpecode;      /* float point exception code */

        /* following field is needed by NLG routines */
        unsigned long   _NLG_dwCode;

        /*
         * Per-Thread data needed by C++ Exception Handling
         */
        void *      _terminate;     /* terminate() routine */
        void *      _unexpected;    /* unexpected() routine */
        void *      _translator;    /* S.E. translator */
        void *      _curexception;  /* current exception */
        void *      _curcontext;    /* current exception context */
#if defined (_M_MRX000)
        void *      _pFrameInfoChain;
        void *      _pUnwindContext;
        void *      _pExitContext;
        int         _MipsPtdDelta;
        int         _MipsPtdEpsilon;
#elif defined (_M_PPC)
        void *      _pExitContext;
        void *      _pUnwindContext;
        void *      _pFrameInfoChain;
        int         _FrameInfo[6];
#endif  /* defined (_M_PPC) */
        };

typedef struct _tiddata * _ptiddata;

/*
 * Declaration of TLS index used in storing pointers to per-thread data
 * structures.
 */
extern unsigned long __tlsindex;

#ifdef _MT

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 * Flag indicating whether or not setlocale() is active. Its value is the
 * number of setlocale() calls currently active.
 */
_CRTIMP extern int __setlc_active;

/*
 * Flag indicating whether or not a function which references the locale
 * without having locked it is active. Its value is the number of such
 * functions.
 */
_CRTIMP extern int __unguarded_readlc_active;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _MT */


/* macros */


#define _lock_fh(fh)            _lock_fhandle(fh)
#define _lock_str(s)            _lock_file(s)
#define _lock_str2(i,s)         _lock_file2(i,s)
#define _lock_fh_check(fh,flag)     if (flag) _lock_fhandle(fh)
#define _mlock(l)               _lock(l)
#define _munlock(l)             _unlock(l)
#define _unlock_fh(fh)          _unlock_fhandle(fh)
#define _unlock_str(s)          _unlock_file(s)
#define _unlock_str2(i,s)       _unlock_file2(i,s)
#define _unlock_fh_check(fh,flag)   if (flag) _unlock_fhandle(fh)

#define _lock_locale(llf)       \
        InterlockedIncrement( &__unguarded_readlc_active );     \
        if ( __setlc_active ) {         \
            InterlockedDecrement( &__unguarded_readlc_active ); \
            _lock( _SETLOCALE_LOCK );   \
            llf = 1;                    \
        }                               \
        else                            \
            llf = 0;

#define _unlock_locale(llf)     \
        if ( llf )                          \
            _unlock( _SETLOCALE_LOCK );     \
        else                                \
            InterlockedDecrement( &__unguarded_readlc_active );



/* multi-thread routines */

void __cdecl _lock(int);
void __cdecl _lock_file(void *);
void __cdecl _lock_file2(int, void *);
void __cdecl _lock_fhandle(int);
void __cdecl _lockexit(void);
void __cdecl _unlock(int);
void __cdecl _unlock_file(void *);
void __cdecl _unlock_file2(int, void *);
void __cdecl _unlock_fhandle(int);
void __cdecl _unlockexit(void);

_ptiddata __cdecl _getptd(void);  /* return address of per-thread CRT data */
void __cdecl _freeptd(_ptiddata); /* free up a per-thread CRT data block */
void __cdecl _initptd(_ptiddata); /* initialize a per-thread CRT data block */


#else  /* defined (_MT) */


/* macros */
#define _lock_fh(fh)
#define _lock_str(s)
#define _lock_str2(i,s)
#define _lock_fh_check(fh,flag)
#define _mlock(l)
#define _munlock(l)
#define _unlock_fh(fh)
#define _unlock_str(s)
#define _unlock_str2(i,s)
#define _unlock_fh_check(fh,flag)

#define _lock_locale(llf)
#define _unlock_locale(llf)

#endif  /* defined (_MT) */

#endif  /* __assembler */


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_MTDLL */
