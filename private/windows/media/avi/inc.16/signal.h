/***
*signal.h - defines signal values and routines
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the signal values and declares the signal functions.
*   [ANSI/System V]
*
****/

#ifndef _INC_SIGNAL

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#ifndef _SIG_ATOMIC_T_DEFINED
typedef int sig_atomic_t;
#define _SIG_ATOMIC_T_DEFINED
#endif 

#define NSIG 23     /* maximum signal number + 1 */


/* signal types */

#ifndef _WINDOWS
#define SIGINT      2   /* Ctrl-C sequence */
#define SIGILL      4   /* illegal instruction - invalid function image */
#endif 
#define SIGFPE      8   /* floating point exception */
#ifndef _WINDOWS
#define SIGSEGV     11  /* segment violation */
#define SIGTERM     15  /* Software termination signal from kill */
#define SIGABRT     22  /* abnormal termination triggered by abort call */
#endif 


/* signal action codes */

/* default signal action */
#define SIG_DFL (void (__cdecl *)(int))0

/* ignore */
#define SIG_IGN (void (__cdecl *)(int))1

/* signal error value (returned by signal call on error) */
#define SIG_ERR (void (__cdecl *)(int))-1


/* function prototypes */

void (__cdecl * __cdecl signal(int,
    void (__cdecl *)(int)))(int);
#ifndef _MT
int __cdecl raise(int);
#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_SIGNAL
#endif 
