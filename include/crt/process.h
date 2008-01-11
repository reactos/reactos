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

_CRTIMP void __cdecl __MINGW_NOTHROW _cexit(void);
_CRTIMP void __cdecl __MINGW_NOTHROW _c_exit(void);

_CRTIMP int __cdecl __MINGW_NOTHROW _cwait (int*, _pid_t, int);

_CRTIMP _pid_t __cdecl __MINGW_NOTHROW _getpid(void);

_CRTIMP int __cdecl __MINGW_NOTHROW _execl	(const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW _execle	(const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW _execlp	(const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW _execlpe	(const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW _execv	(const char*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW _execve	(const char*, const char* const*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW _execvp	(const char*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW _execvpe	(const char*, const char* const*, const char* const*);

_CRTIMP int __cdecl __MINGW_NOTHROW _spawnl	(int, const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW _spawnle	(int, const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW _spawnlp	(int, const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW _spawnlpe	(int, const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW _spawnv	(int, const char*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW _spawnve	(int, const char*, const char* const*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW _spawnvp	(int, const char*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW _spawnvpe	(int, const char*, const char* const*, const char* const*);


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
_CRTIMP unsigned long __cdecl __MINGW_NOTHROW
	_beginthread	(void (*)(void *), unsigned, void*);
_CRTIMP void __cdecl __MINGW_NOTHROW _endthread	(void);

#ifdef	__MSVCRT__
_CRTIMP unsigned long __cdecl __MINGW_NOTHROW
	_beginthreadex	(void *, unsigned, unsigned (__stdcall *) (void *),
			 void*, unsigned, unsigned*);
_CRTIMP void __cdecl __MINGW_NOTHROW _endthreadex (unsigned);
#endif


#ifndef	_NO_OLDNAMES
/*
 * Functions without the leading underscore, for portability. These functions
 * live in liboldnames.a.
 */
_CRTIMP int  __cdecl __MINGW_NOTHROW cwait (int*, pid_t, int);
_CRTIMP pid_t __cdecl __MINGW_NOTHROW getpid (void);
_CRTIMP int __cdecl __MINGW_NOTHROW execl (const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW execle (const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW execlp (const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW execlpe (const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW execv (const char*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW execve (const char*, const char* const*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW execvp (const char*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW execvpe (const char*, const char* const*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW spawnl (int, const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW spawnle (int, const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW spawnlp (int, const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW spawnlpe (int, const char*, const char*, ...);
_CRTIMP int __cdecl __MINGW_NOTHROW spawnv (int, const char*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW spawnve (int, const char*, const char* const*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW spawnvp (int, const char*, const char* const*);
_CRTIMP int __cdecl __MINGW_NOTHROW spawnvpe (int, const char*, const char* const*, const char* const*);
#endif	/* Not _NO_OLDNAMES */

#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* _PROCESS_H_ not defined */
