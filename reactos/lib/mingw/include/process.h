/* 
 * process.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Function calls for spawning child processes.
 *
 */

#ifndef	_PROCESS_H_
#define	_PROCESS_H_

/* All the headers include this file. */
#include <_mingw.h>

/* Includes a definition of _pid_t and pid_t */
#include <sys/types.h>

/*
 * Constants for cwait actions.
 * Obsolete for Win32.
 */
#define	_WAIT_CHILD		0
#define	_WAIT_GRANDCHILD	1

#ifndef	_NO_OLDNAMES
#define	WAIT_CHILD		_WAIT_CHILD
#define	WAIT_GRANDCHILD		_WAIT_GRANDCHILD
#endif	/* Not _NO_OLDNAMES */

/*
 * Mode constants for spawn functions.
 */
#define	_P_WAIT		0
#define	_P_NOWAIT	1
#define	_P_OVERLAY	2
#define	_OLD_P_OVERLAY	_P_OVERLAY
#define	_P_NOWAITO	3
#define	_P_DETACH	4

#ifndef	_NO_OLDNAMES
#define	P_WAIT		_P_WAIT
#define	P_NOWAIT	_P_NOWAIT
#define	P_OVERLAY	_P_OVERLAY
#define	OLD_P_OVERLAY	_OLD_P_OVERLAY
#define	P_NOWAITO	_P_NOWAITO
#define	P_DETACH	_P_DETACH
#endif	/* Not _NO_OLDNAMES */


#ifndef RC_INVOKED

#ifdef	__cplusplus
extern "C" {
#endif

_CRTIMP void __cdecl _cexit(void);
_CRTIMP void __cdecl _c_exit(void);

_CRTIMP int __cdecl _cwait (int*, _pid_t, int);

_CRTIMP _pid_t __cdecl _getpid(void);

_CRTIMP int __cdecl _execl	(const char*, const char*, ...);
_CRTIMP int __cdecl _execle	(const char*, const char*, ...);
_CRTIMP int __cdecl _execlp	(const char*, const char*, ...);
_CRTIMP int __cdecl _execlpe	(const char*, const char*, ...);
_CRTIMP int __cdecl _execv	(const char*, const char* const*);
_CRTIMP int __cdecl _execve	(const char*, const char* const*, const char* const*);
_CRTIMP int __cdecl _execvp	(const char*, const char* const*);
_CRTIMP int __cdecl _execvpe	(const char*, const char* const*, const char* const*);

_CRTIMP int __cdecl _spawnl	(int, const char*, const char*, ...);
_CRTIMP int __cdecl _spawnle	(int, const char*, const char*, ...);
_CRTIMP int __cdecl _spawnlp	(int, const char*, const char*, ...);
_CRTIMP int __cdecl _spawnlpe	(int, const char*, const char*, ...);
_CRTIMP int __cdecl _spawnv	(int, const char*, const char* const*);
_CRTIMP int __cdecl _spawnve	(int, const char*, const char* const*, const char* const*);
_CRTIMP int __cdecl _spawnvp	(int, const char*, const char* const*);
_CRTIMP int __cdecl _spawnvpe	(int, const char*, const char* const*, const char* const*);


/*
 * The functions _beginthreadex and _endthreadex are not provided by CRTDLL.
 * They are provided by MSVCRT.
 *
 * NOTE: Apparently _endthread calls CloseHandle on the handle of the thread,
 * making for race conditions if you are not careful. Basically you have to
 * make sure that no-one is going to do *anything* with the thread handle
 * after the thread calls _endthread or returns from the thread function.
 *
 * NOTE: No old names for these functions. Use the underscore.
 */
_CRTIMP unsigned long __cdecl
	_beginthread	(void (*)(void *), unsigned, void*);
_CRTIMP void __cdecl _endthread	(void);

#ifdef	__MSVCRT__
_CRTIMP unsigned long __cdecl
	_beginthreadex	(void *, unsigned, unsigned (__stdcall *) (void *), 
			 void*, unsigned, unsigned*);
_CRTIMP void __cdecl _endthreadex (unsigned);
#endif


#ifndef	_NO_OLDNAMES
/*
 * Functions without the leading underscore, for portability. These functions
 * live in liboldnames.a.
 */
_CRTIMP int  __cdecl cwait (int*, pid_t, int);
_CRTIMP pid_t __cdecl getpid (void);
_CRTIMP int __cdecl execl (const char*, const char*, ...);
_CRTIMP int __cdecl execle (const char*, const char*, ...);
_CRTIMP int __cdecl execlp (const char*, const char*, ...);
_CRTIMP int __cdecl execlpe (const char*, const char*, ...);
_CRTIMP int __cdecl execv (const char*, const char* const*);
_CRTIMP int __cdecl execve (const char*, const char* const*, const char* const*);
_CRTIMP int __cdecl execvp (const char*, const char* const*);
_CRTIMP int __cdecl execvpe (const char*, const char* const*, const char* const*);
_CRTIMP int __cdecl spawnl (int, const char*, const char*, ...);
_CRTIMP int __cdecl spawnle (int, const char*, const char*, ...);
_CRTIMP int __cdecl spawnlp (int, const char*, const char*, ...);
_CRTIMP int __cdecl spawnlpe (int, const char*, const char*, ...);
_CRTIMP int __cdecl spawnv (int, const char*, const char* const*);
_CRTIMP int __cdecl spawnve (int, const char*, const char* const*, const char* const*);
_CRTIMP int __cdecl spawnvp (int, const char*, const char* const*);
_CRTIMP int __cdecl spawnvpe (int, const char*, const char* const*, const char* const*);
#endif	/* Not _NO_OLDNAMES */

#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* _PROCESS_H_ not defined */
