/* 
 * process.h
 *
 * Function calls for spawning child processes.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.2 $
 * $Author: rex $
 * $Date: 1999/03/19 05:55:09 $
 *
 */
/* Appropriated for Reactos Crtdll by Ariadne */

#ifndef	_PROCESS_H_
#define	_PROCESS_H_

#ifndef	__STRICT_ANSI__

#ifdef	__cplusplus
extern "C" {
#endif

void	_cexit(void);
void	_c_exit(void);

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

int	_cwait (int* pnStatus, int nPID, int nAction);

int	_getpid(void);

int _execl(const char *_path, const char *_argv0, ...);
int _execle(const char *_path, const char *_argv0, ... /*, char *const _envp[] */);
int _execlp(const char *_path, const char *_argv0, ...);
int _execlpe(const char *_path, const char *_argv0, ... /*, char *const _envp[] */);

int _execv(const char *_path,const char *const _argv[]);
int _execve(const char *_path,const char *const _argv[],const char *const _envp[]);
int _execvp(const char *_path,const char *const _argv[]);
int _execvpe(const char *_path,const char *const _argv[],const char *const _envp[]);




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

int _spawnl(int _mode, const char *_path, const char *_argv0, ...);
int _spawnle(int _mode, const char *_path, const char *_argv0, ... /*, char *const _envp[] */);
int _spawnlp(int _mode, const char *_path, const char *_argv0, ...);
int _spawnlpe(int _mode, const char *_path, const char *_argv0, ... /*, char *const _envp[] */);

int _spawnv(int _mode, const char *_path,const char *const _argv[]);
int _spawnve(int _mode, const char *_path,const char *const _argv[],const char *const _envp[]);
int _spawnvp(int _mode, const char *_path,const char *const _argv[]);
int _spawnvpe(int _mode, const char *_path,const char *const _argv[],const char *const _envp[]);

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
unsigned long
	_beginthread	(void (*pfuncStart)(void *),
			 unsigned unStackSize, void* pArgList);
void	_endthread	(void);

#if	__MSVCRT__
unsigned long
	_beginthreadex	(void *pSecurity, unsigned unStackSize,
			 unsigned (*pfuncStart)(void*), void* pArgList,
			 unsigned unInitFlags, unsigned* pThreadAddr);
void	_endthreadex	(unsigned unExitCode);
#endif


void *_loaddll (char *name);
int _unloaddll(void *handle);

#ifndef	_NO_OLDNAMES

#define cwait		_cwait
#define getpid          _getpid
#define	execl		_execl        
#define execle         _execle       
#define execlp         _execlp       
#define execlpe        _execlpe      
                              
#define execv          _execv        
#define execve         _execve       
#define execvp         _execvp       
#define execvpe        _execvpe   

#define	spawnl		_spawnl        
#define spawnle         _spawnle       
#define spawnlp         _spawnlp       
#define spawnlpe        _spawnlpe      
                              
#define spawnv          _spawnv        
#define spawnve         _spawnve       
#define spawnvp         _spawnvp       
#define spawnvpe        _spawnvpe    
                              

#endif	/* Not _NO_OLDNAMES */

#ifdef	__cplusplus
}
#endif

#endif	/* Not __STRICT_ANSI__ */

#endif	/* _PROCESS_H_ not defined */
