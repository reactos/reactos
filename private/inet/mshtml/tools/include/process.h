/***
*process.h - definition and declarations for process control functions
*
*	Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file defines the modeflag values for spawnxx calls.
*	Also contains the function argument declarations for all
*	process control related routines.
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_PROCESS
#define _INC_PROCESS

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifndef _POSIX_

#ifdef __cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if	_MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if	_MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else	/* ndef _NTSDK */
/* current definition */
#ifdef	_DLL
#define _CRTIMP __declspec(dllimport)
#else	/* ndef _DLL */
#define _CRTIMP
#endif	/* _DLL */
#endif	/* _NTSDK */
#endif	/* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if	( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


#ifndef _MAC
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif /* ndef _MAC */


/* modeflag values for _spawnxx routines */

#ifndef _MAC

#define _P_WAIT 	0
#define _P_NOWAIT	1
#define _OLD_P_OVERLAY	2
#define _P_NOWAITO	3
#define _P_DETACH	4

#ifdef _MT
#define _P_OVERLAY	2
#else
extern int _p_overlay;
#define _P_OVERLAY	_p_overlay
#endif	/* _MT */

/* Action codes for _cwait(). The action code argument to _cwait is ignored
   on Win32 though it is accepted for compatibilty with old MS CRT libs */
#define _WAIT_CHILD	 0
#define _WAIT_GRANDCHILD 1

#else /* ndef _MAC */

#define _P_NOWAIT	1
#define _P_OVERLAY	2

#endif /* ndef _MAC */


/* function prototypes */

#ifdef	_MT
_CRTIMP unsigned long  __cdecl _beginthread (void (__cdecl *) (void *),
	unsigned, void *);
_CRTIMP void __cdecl _endthread(void);
_CRTIMP unsigned long __cdecl _beginthreadex(void *, unsigned,
	unsigned (__stdcall *) (void *), void *, unsigned, unsigned *);
_CRTIMP void __cdecl _endthreadex(unsigned);
#endif

_CRTIMP void __cdecl abort(void);
_CRTIMP void __cdecl _cexit(void);
_CRTIMP void __cdecl _c_exit(void);
_CRTIMP void __cdecl exit(int);
_CRTIMP void __cdecl _exit(int);
_CRTIMP int __cdecl _getpid(void);

#ifndef _MAC

_CRTIMP int __cdecl _cwait(int *, int, int);
_CRTIMP int __cdecl _execl(const char *, const char *, ...);
_CRTIMP int __cdecl _execle(const char *, const char *, ...);
_CRTIMP int __cdecl _execlp(const char *, const char *, ...);
_CRTIMP int __cdecl _execlpe(const char *, const char *, ...);
_CRTIMP int __cdecl _execv(const char *, const char * const *);
_CRTIMP int __cdecl _execve(const char *, const char * const *, const char * const *);
_CRTIMP int __cdecl _execvp(const char *, const char * const *);
_CRTIMP int __cdecl _execvpe(const char *, const char * const *, const char * const *);
_CRTIMP int __cdecl _spawnl(int, const char *, const char *, ...);
_CRTIMP int __cdecl _spawnle(int, const char *, const char *, ...);
_CRTIMP int __cdecl _spawnlp(int, const char *, const char *, ...);
_CRTIMP int __cdecl _spawnlpe(int, const char *, const char *, ...);
_CRTIMP int __cdecl _spawnv(int, const char *, const char * const *);
_CRTIMP int __cdecl _spawnve(int, const char *, const char * const *,
	const char * const *);
_CRTIMP int __cdecl _spawnvp(int, const char *, const char * const *);
_CRTIMP int __cdecl _spawnvpe(int, const char *, const char * const *,
	const char * const *);
_CRTIMP int __cdecl system(const char *);

#else /* ndef _MAC */

_CRTIMP int __cdecl _spawn(int, const char *);

#endif /* ndef _MAC */

#ifndef _MAC
#ifndef _WPROCESS_DEFINED
/* wide function prototypes, also declared in wchar.h  */
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

/* --------- The following functions are OBSOLETE --------- */
/*
 * The Win32 API LoadLibrary, FreeLibrary and GetProcAddress should be used
 * instead.
 */
int __cdecl _loaddll(char *);
int __cdecl _unloaddll(int);
int (__cdecl * __cdecl _getdllprocaddr(int, char *, int))();
/* --------- The preceding functions are OBSOLETE --------- */


#ifdef	_DECL_DLLMAIN
/*
 * Declare DLL notification (initialization/termination) routines
 *	The preferred method is for the user to provide DllMain() which will
 *	be called automatically by the DLL entry point defined by the C run-
 *	time library code.  If the user wants to define the DLL entry point
 *	routine, the user's entry point must call _CRT_INIT on all types of
 *	notifications, as the very first thing on attach notifications and
 *	as the very last thing on detach notifications.
 */
#ifdef	_WINDOWS_	/* Use types from WINDOWS.H */
BOOL WINAPI DllMain(HANDLE, DWORD, LPVOID);
BOOL WINAPI _CRT_INIT(HANDLE, DWORD, LPVOID);
BOOL WINAPI _wCRT_INIT(HANDLE, DWORD, LPVOID);
extern BOOL (WINAPI *_pRawDllMain)(HANDLE, DWORD, LPVOID);
#else
int __stdcall DllMain(void *, unsigned, void *);
int __stdcall _CRT_INIT(void *, unsigned, void *);
int __stdcall _wCRT_INIT(void *, unsigned, void *);
extern int (__stdcall *_pRawDllMain)(void *, unsigned, void *);
#endif	/* _WINDOWS_ */
#endif
#endif /* ndef _MAC */

#if	!__STDC__

/* Non-ANSI names for compatibility */


#ifndef _MAC

#define P_WAIT		_P_WAIT
#define P_NOWAIT	_P_NOWAIT
#define P_OVERLAY	_P_OVERLAY
#define OLD_P_OVERLAY	_OLD_P_OVERLAY
#define P_NOWAITO	_P_NOWAITO
#define P_DETACH	_P_DETACH
#define WAIT_CHILD	_WAIT_CHILD
#define WAIT_GRANDCHILD _WAIT_GRANDCHILD

#else /* ndef _MAC */

#define P_NOWAIT	_P_NOWAIT
#define P_OVERLAY	_P_OVERLAY

#endif /* ndef _MAC */

#ifdef	_NTSDK

/* definitions compatible with NT SDK */
#define cwait	 _cwait
#define execl	 _execl
#define execle	 _execle
#define execlp	 _execlp
#define execlpe  _execlpe
#define execv	 _execv
#define execve	 _execve
#define execvp	 _execvp
#define execvpe  _execvpe
#define getpid	 _getpid
#define spawnl	 _spawnl
#define spawnle  _spawnle
#define spawnlp  _spawnlp
#define spawnlpe _spawnlpe
#define spawnv	 _spawnv
#define spawnve  _spawnve
#define spawnvp  _spawnvp
#define spawnvpe _spawnvpe

#else	/* ndef _NTSDK */

#ifndef _MAC

/* current declarations */
_CRTIMP int __cdecl cwait(int *, int, int);
_CRTIMP int __cdecl execl(const char *, const char *, ...);
_CRTIMP int __cdecl execle(const char *, const char *, ...);
_CRTIMP int __cdecl execlp(const char *, const char *, ...);
_CRTIMP int __cdecl execlpe(const char *, const char *, ...);
_CRTIMP int __cdecl execv(const char *, const char * const *);
_CRTIMP int __cdecl execve(const char *, const char * const *, const char * const *);
_CRTIMP int __cdecl execvp(const char *, const char * const *);
_CRTIMP int __cdecl execvpe(const char *, const char * const *, const char * const *);
_CRTIMP int __cdecl spawnl(int, const char *, const char *, ...);
_CRTIMP int __cdecl spawnle(int, const char *, const char *, ...);
_CRTIMP int __cdecl spawnlp(int, const char *, const char *, ...);
_CRTIMP int __cdecl spawnlpe(int, const char *, const char *, ...);
_CRTIMP int __cdecl spawnv(int, const char *, const char * const *);
_CRTIMP int __cdecl spawnve(int, const char *, const char * const *,
	const char * const *);
_CRTIMP int __cdecl spawnvp(int, const char *, const char * const *);
_CRTIMP int __cdecl spawnvpe(int, const char *, const char * const *,
	const char * const *);

#endif /* ndef _MAC */

_CRTIMP int __cdecl getpid(void);

#endif	/* _NTSDK */

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif	/* _POSIX_ */

#endif	/* _INC_PROCESS */
