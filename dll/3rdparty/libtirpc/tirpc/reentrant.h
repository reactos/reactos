/*-
 * Copyright (c) 1997,98 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by J.T. Conklin.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/libc/include/reentrant.h,v 1.2 2002/11/01 09:37:17 dfr Exp $
 */

/*
 * Requirements:
 * 
 * 1. The thread safe mechanism should be lightweight so the library can
 *    be used by non-threaded applications without unreasonable overhead.
 * 
 * 2. There should be no dependency on a thread engine for non-threaded
 *    applications.
 * 
 * 3. There should be no dependency on any particular thread engine.
 * 
 * 4. The library should be able to be compiled without support for thread
 *    safety.
 * 
 * 
 * Rationale:
 * 
 * One approach for thread safety is to provide discrete versions of the
 * library: one thread safe, the other not.  The disadvantage of this is
 * that libc is rather large, and two copies of a library which are 99%+
 * identical is not an efficent use of resources.
 * 
 * Another approach is to provide a single thread safe library.  However,
 * it should not add significant run time or code size overhead to non-
 * threaded applications.
 * 
 * Since the NetBSD C library is used in other projects, it should be
 * easy to replace the mutual exclusion primitives with ones provided by
 * another system.  Similarly, it should also be easy to remove all
 * support for thread safety completely if the target environment does
 * not support threads.
 * 
 * 
 * Implementation Details:
 * 
 * The mutex primitives used by the library (mutex_t, mutex_lock, etc.)
 * are macros which expand to the cooresponding primitives provided by
 * the thread engine or to nothing.  The latter is used so that code is
 * not unreasonably cluttered with #ifdefs when all thread safe support
 * is removed.
 * 
 * The mutex macros can be directly mapped to the mutex primitives from
 * pthreads, however it should be reasonably easy to wrap another mutex
 * implementation so it presents a similar interface.
 * 
 * Stub implementations of the mutex functions are provided with *weak*
 * linkage.  These functions simply return success.  When linked with a
 * thread library (i.e. -lpthread), the functions will override the
 * stubs.
 */

/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#ifndef _REENTRANT_H
#define _REENTRANT_H
//#include <pthread.h>
#include <libc_private.h>

#include <stdlib.h>


#define mutex_t				CRITICAL_SECTION
#define cond_t				CONDITION_VARIABLE
#define rwlock_t			SRWLOCK


#define thread_key_t		DWORD
#define MUTEX_INITIALIZER	-1 /*THIS_NEEDS_HELP*/
#define RWLOCK_INITIALIZER	-1 /*THIS_NEEDS_HELP*/
#define mutex_init(m, a)	InitializeCriticalSection(m)
#define mutex_lock(m)		EnterCriticalSection(m)
#define mutex_unlock(m)		LeaveCriticalSection(m)
#define mutex_trylock(m)	TryEnterCriticalSection(m)

#define cond_init(c, a, p)		InitializeConditionVariable(c)
#define cond_signal(m)			WakeConditionVariable(m)
#define cond_broadcast(m)		WakeAllConditionVariable(m)
#define cond_wait(c, m)			SleepConditionVariableCS(c, m, INFINITE)
#define cond_wait_timed(c, m, t) SleepConditionVariableCS(c, m, t)

#define rwlock_init(l, a)		InitializeSRWLock(l)
#define rwlock_rdlock(l)		AcquireSRWLockShared(l)
#define rwlock_wrlock(l)		AcquireSRWLockExclusive(l)
/* XXX Code will have to be changed to release the right kind!!! XXX */
#define rwlock_unlock(l)		ReleaseSRWLockExclusive(l)

#define thr_keycreate(k, d)		((*k) = TlsAlloc())
#define thr_keydelete(k)		TlsFree(k)
#define thr_setspecific(k, p)	TlsSetValue(k, p)
#define thr_getspecific(k)		TlsGetValue(k)
#define thr_sigsetmask(f, n, o)	 dunno_sigmask(f, n, o)

#define thr_self()				GetCurrentThreadId()
#define thr_exit(x)				ExitThread(x)

/*
#define mutex_t			pthread_mutex_t
#define cond_t			pthread_cond_t
#define rwlock_t		pthread_rwlock_t

#define thread_key_t		pthread_key_t
#define MUTEX_INITIALIZER	PTHREAD_MUTEX_INITIALIZER
#define RWLOCK_INITIALIZER	PTHREAD_RWLOCK_INITIALIZER
#define mutex_init(m, a)	pthread_mutex_init(m, a)
#define mutex_lock(m)		pthread_mutex_lock(m)
#define mutex_unlock(m)		pthread_mutex_unlock(m)
#define mutex_trylock(m)	pthread_mutex_trylock(m)

#define cond_init(c, a, p)	 pthread_cond_init(c, a)
#define cond_signal(m)		 pthread_cond_signal(m)
#define cond_broadcast(m)	 pthread_cond_broadcast(m)
#define cond_wait(c, m)		 pthread_cond_wait(c, m)

#define rwlock_init(l, a)        pthread_rwlock_init(l, a)
#define rwlock_rdlock(l)	 pthread_rwlock_rdlock(l)
#define rwlock_wrlock(l)	 pthread_rwlock_wrlock(l)
#define rwlock_unlock(l)	 pthread_rwlock_unlock(l)

#define thr_keycreate(k, d)	 pthread_key_create(k, d)
#define thr_keydelete(k)	 pthread_key_delete(k)
#define thr_setspecific(k, p)	 pthread_setspecific(k, p)
#define thr_getspecific(k)	 pthread_getspecific(k)
#define thr_sigsetmask(f, n, o)	 pthread_sigmask(f, n, o)

#define thr_self()		 pthread_self()
#define thr_exit(x)		 pthread_exit(x)
*/
#endif /* reentrant.h */
