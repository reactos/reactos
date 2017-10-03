/*
 * Copyright (c) 1998 John Birrell <jb@cimlogic.com.au>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by John Birrell.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JOHN BIRRELL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/libc/include/libc_private.h,v 1.11 2003/11/05 18:17:30 deischen Exp $
 *
 * Private definitions for libc, libc_r and libpthread.
 *
 */

#ifndef _LIBC_PRIVATE_H_
#define _LIBC_PRIVATE_H_

/*
 * This global flag is non-zero when a process has created one
 * or more threads. It is used to avoid calling locking functions
 * when they are not required.
 */
extern int	__isthreaded;

/*
 * File lock contention is difficult to diagnose without knowing
 * where locks were set. Allow a debug library to be built which
 * records the source file and line number of each lock call.
 */
#ifdef	_FLOCK_DEBUG
#define _FLOCKFILE(x)	_flockfile_debug(x, __FILE__, __LINE__)
#else
#define _FLOCKFILE(x)	_flockfile(x)
#endif

/*
 * Macros for locking and unlocking FILEs. These test if the
 * process is threaded to avoid locking when not required.
 */
#define	FLOCKFILE(fp)		if (__isthreaded) _FLOCKFILE(fp)
#define	FUNLOCKFILE(fp)		if (__isthreaded) _funlockfile(fp)

/*
 * Indexes into the pthread jump table.
 *
 * Warning! If you change this type, you must also change the threads
 * libraries that reference it (libc_r, libpthread).
 */
typedef enum {
	PJT_COND_BROADCAST,
	PJT_COND_DESTROY,
	PJT_COND_INIT,
	PJT_COND_SIGNAL,
	PJT_COND_WAIT,
	PJT_GETSPECIFIC,
	PJT_KEY_CREATE,
	PJT_KEY_DELETE,
	PJT_MAIN_NP,
	PJT_MUTEX_DESTROY,
	PJT_MUTEX_INIT,
	PJT_MUTEX_LOCK,
	PJT_MUTEX_TRYLOCK,
	PJT_MUTEX_UNLOCK,
	PJT_MUTEXATTR_DESTROY,
	PJT_MUTEXATTR_INIT,
	PJT_MUTEXATTR_SETTYPE,
	PJT_ONCE,
	PJT_RWLOCK_DESTROY,
	PJT_RWLOCK_INIT,
	PJT_RWLOCK_RDLOCK,
	PJT_RWLOCK_TRYRDLOCK,
	PJT_RWLOCK_TRYWRLOCK,
	PJT_RWLOCK_UNLOCK,
	PJT_RWLOCK_WRLOCK,
	PJT_SELF,
	PJT_SETSPECIFIC,
	PJT_SIGMASK,
	PJT_MAX
} pjt_index_t;

typedef int (*pthread_func_t)(void);
typedef pthread_func_t pthread_func_entry_t[2];

extern pthread_func_entry_t __thr_jtable[];

/*
 * yplib internal interfaces
 */
#ifdef YP
int _yp_check(char **);
#endif


/*
 * This is a pointer in the C run-time startup code. It is used
 * by getprogname() and setprogname().
 */
extern const char *__progname;

/*
 * This is the lock to make malloc() thread-safe.  It is externalized
 * so that thread libraries can protect malloc across fork().
 */
extern struct _spinlock *__malloc_lock;

#endif /* _LIBC_PRIVATE_H_ */
