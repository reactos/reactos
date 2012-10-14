/**************************************************************************
 * 
 * Copyright 1999-2006 Brian Paul
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * @file
 * 
 * Thread, mutex, condition variable, barrier, semaphore and
 * thread-specific data functions.
 */


#ifndef OS_THREAD_H_
#define OS_THREAD_H_


#include "pipe/p_compiler.h"
#include "util/u_debug.h" /* for assert */


#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU) || defined(PIPE_OS_CYGWIN)

#include <pthread.h> /* POSIX threads headers */
#include <stdio.h> /* for perror() */


/* pipe_thread
 */
typedef pthread_t pipe_thread;

#define PIPE_THREAD_ROUTINE( name, param ) \
   void *name( void *param )

static INLINE pipe_thread pipe_thread_create( void *(* routine)( void *), void *param )
{
   pipe_thread thread;
   sigset_t saved_set, new_set;
   int ret;

   sigfillset(&new_set);
   pthread_sigmask(SIG_SETMASK, &new_set, &saved_set);
   ret = pthread_create( &thread, NULL, routine, param );
   pthread_sigmask(SIG_SETMASK, &saved_set, NULL);
   if (ret)
      return 0;
   return thread;
}

static INLINE int pipe_thread_wait( pipe_thread thread )
{
   return pthread_join( thread, NULL );
}

static INLINE int pipe_thread_destroy( pipe_thread thread )
{
   return pthread_detach( thread );
}


/* pipe_mutex
 */
typedef pthread_mutex_t pipe_mutex;

#define pipe_static_mutex(mutex) \
   static pipe_mutex mutex = PTHREAD_MUTEX_INITIALIZER

#define pipe_mutex_init(mutex) \
   (void) pthread_mutex_init(&(mutex), NULL)

#define pipe_mutex_destroy(mutex) \
   pthread_mutex_destroy(&(mutex))

#define pipe_mutex_lock(mutex) \
   (void) pthread_mutex_lock(&(mutex))

#define pipe_mutex_unlock(mutex) \
   (void) pthread_mutex_unlock(&(mutex))


/* pipe_condvar
 */
typedef pthread_cond_t pipe_condvar;

#define pipe_static_condvar(mutex) \
   static pipe_condvar mutex = PTHREAD_COND_INITIALIZER

#define pipe_condvar_init(cond)	\
   pthread_cond_init(&(cond), NULL)

#define pipe_condvar_destroy(cond) \
   pthread_cond_destroy(&(cond))

#define pipe_condvar_wait(cond, mutex) \
  pthread_cond_wait(&(cond), &(mutex))

#define pipe_condvar_signal(cond) \
  pthread_cond_signal(&(cond))

#define pipe_condvar_broadcast(cond) \
  pthread_cond_broadcast(&(cond))



#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)

#include <windows.h>

/* pipe_thread
 */
typedef HANDLE pipe_thread;

#define PIPE_THREAD_ROUTINE( name, param ) \
   void * WINAPI name( void *param )

static INLINE pipe_thread pipe_thread_create( void *(WINAPI * routine)( void *), void *param )
{
   DWORD id;
   return CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) routine, param, 0, &id );
}

static INLINE int pipe_thread_wait( pipe_thread thread )
{
   if (WaitForSingleObject( thread, INFINITE ) == WAIT_OBJECT_0)
      return 0;
   return -1;
}

static INLINE int pipe_thread_destroy( pipe_thread thread )
{
   if (CloseHandle( thread ))
      return 0;
   return -1;
}


/* pipe_mutex
 */
typedef CRITICAL_SECTION pipe_mutex;

/* http://locklessinc.com/articles/pthreads_on_windows/ */
#define pipe_static_mutex(mutex) \
   static pipe_mutex mutex = {(PCRITICAL_SECTION_DEBUG)-1, -1, 0, 0, 0, 0}

#define pipe_mutex_init(mutex) \
   InitializeCriticalSection(&mutex)

#define pipe_mutex_destroy(mutex) \
   DeleteCriticalSection(&mutex)

#define pipe_mutex_lock(mutex) \
   EnterCriticalSection(&mutex)

#define pipe_mutex_unlock(mutex) \
   LeaveCriticalSection(&mutex)

/* TODO: Need a macro to declare "I don't care about WinXP compatibilty" */
#if 0 && defined (_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
/* CONDITION_VARIABLE is only available on newer versions of Windows
 * (Server 2008/Vista or later).
 * http://msdn.microsoft.com/en-us/library/ms682052(VS.85).aspx
 *
 * pipe_condvar
 */
typedef CONDITION_VARIABLE pipe_condvar;

#define pipe_static_condvar(cond) \
   /*static*/ pipe_condvar cond = CONDITION_VARIABLE_INIT

#define pipe_condvar_init(cond) \
   InitializeConditionVariable(&(cond))

#define pipe_condvar_destroy(cond) \
   (void) cond /* nothing to do */

#define pipe_condvar_wait(cond, mutex) \
   SleepConditionVariableCS(&(cond), &(mutex), INFINITE)

#define pipe_condvar_signal(cond) \
   WakeConditionVariable(&(cond))

#define pipe_condvar_broadcast(cond) \
   WakeAllConditionVariable(&(cond))

#else /* need compatibility with pre-Vista Win32 */

/* pipe_condvar (XXX FIX THIS)
 * See http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
 * for potential pitfalls in implementation.
 */
typedef DWORD pipe_condvar;

#define pipe_static_condvar(cond) \
   /*static*/ pipe_condvar cond = 1

#define pipe_condvar_init(cond) \
   (void) (cond = 1)

#define pipe_condvar_destroy(cond) \
   (void) cond

/* Poor man's pthread_cond_wait():
   Just release the mutex and sleep for one millisecond.
   The caller's while() loop does all the work. */
#define pipe_condvar_wait(cond, mutex) \
   do { pipe_mutex_unlock(mutex); \
        Sleep(cond); \
        pipe_mutex_lock(mutex); \
   } while (0)

#define pipe_condvar_signal(cond) \
   (void) cond

#define pipe_condvar_broadcast(cond) \
   (void) cond

#endif /* pre-Vista win32 */

#else

#include "os/os_time.h"

/** Dummy definitions */

typedef unsigned pipe_thread;

#define PIPE_THREAD_ROUTINE( name, param ) \
   void * name( void *param )

static INLINE pipe_thread pipe_thread_create( void *(* routine)( void *), void *param )
{
   return 0;
}

static INLINE int pipe_thread_wait( pipe_thread thread )
{
   return -1;
}

static INLINE int pipe_thread_destroy( pipe_thread thread )
{
   return -1;
}

typedef unsigned pipe_mutex;

#define pipe_static_mutex(mutex) \
   static pipe_mutex mutex = 0

#define pipe_mutex_init(mutex) \
   (void) mutex

#define pipe_mutex_destroy(mutex) \
   (void) mutex

#define pipe_mutex_lock(mutex) \
   (void) mutex

#define pipe_mutex_unlock(mutex) \
   (void) mutex

typedef int64_t pipe_condvar;

#define pipe_static_condvar(condvar) \
   static pipe_condvar condvar = 1000

#define pipe_condvar_init(condvar) \
   (void) (condvar = 1000)

#define pipe_condvar_destroy(condvar) \
   (void) condvar

/* Poor man's pthread_cond_wait():
   Just release the mutex and sleep for one millisecond.
   The caller's while() loop does all the work. */
#define pipe_condvar_wait(condvar, mutex) \
   do { pipe_mutex_unlock(mutex); \
        os_time_sleep(condvar); \
        pipe_mutex_lock(mutex); \
   } while (0)

#define pipe_condvar_signal(condvar) \
   (void) condvar

#define pipe_condvar_broadcast(condvar) \
   (void) condvar


#endif  /* PIPE_OS_? */


/*
 * pipe_barrier
 */

#if (defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS)) && !defined(PIPE_OS_ANDROID)

typedef pthread_barrier_t pipe_barrier;

static INLINE void pipe_barrier_init(pipe_barrier *barrier, unsigned count)
{
   pthread_barrier_init(barrier, NULL, count);
}

static INLINE void pipe_barrier_destroy(pipe_barrier *barrier)
{
   pthread_barrier_destroy(barrier);
}

static INLINE void pipe_barrier_wait(pipe_barrier *barrier)
{
   pthread_barrier_wait(barrier);
}


#else /* If the OS doesn't have its own, implement barriers using a mutex and a condvar */

typedef struct {
   unsigned count;
   unsigned waiters;
   uint64_t sequence;
   pipe_mutex mutex;
   pipe_condvar condvar;
} pipe_barrier;

static INLINE void pipe_barrier_init(pipe_barrier *barrier, unsigned count)
{
   barrier->count = count;
   barrier->waiters = 0;
   barrier->sequence = 0;
   pipe_mutex_init(barrier->mutex);
   pipe_condvar_init(barrier->condvar);
}

static INLINE void pipe_barrier_destroy(pipe_barrier *barrier)
{
   assert(barrier->waiters == 0);
   pipe_mutex_destroy(barrier->mutex);
   pipe_condvar_destroy(barrier->condvar);
}

static INLINE void pipe_barrier_wait(pipe_barrier *barrier)
{
   pipe_mutex_lock(barrier->mutex);

   assert(barrier->waiters < barrier->count);
   barrier->waiters++;

   if (barrier->waiters < barrier->count) {
      uint64_t sequence = barrier->sequence;

      do {
         pipe_condvar_wait(barrier->condvar, barrier->mutex);
      } while (sequence == barrier->sequence);
   } else {
      barrier->waiters = 0;
      barrier->sequence++;
      pipe_condvar_broadcast(barrier->condvar);
   }

   pipe_mutex_unlock(barrier->mutex);
}


#endif


/*
 * Semaphores
 */

typedef struct
{
   pipe_mutex mutex;
   pipe_condvar cond;
   int counter;
} pipe_semaphore;


static INLINE void
pipe_semaphore_init(pipe_semaphore *sema, int init_val)
{
   pipe_mutex_init(sema->mutex);
   pipe_condvar_init(sema->cond);
   sema->counter = init_val;
}

static INLINE void
pipe_semaphore_destroy(pipe_semaphore *sema)
{
   pipe_mutex_destroy(sema->mutex);
   pipe_condvar_destroy(sema->cond);
}

/** Signal/increment semaphore counter */
static INLINE void
pipe_semaphore_signal(pipe_semaphore *sema)
{
   pipe_mutex_lock(sema->mutex);
   sema->counter++;
   pipe_condvar_signal(sema->cond);
   pipe_mutex_unlock(sema->mutex);
}

/** Wait for semaphore counter to be greater than zero */
static INLINE void
pipe_semaphore_wait(pipe_semaphore *sema)
{
   pipe_mutex_lock(sema->mutex);
   while (sema->counter <= 0) {
      pipe_condvar_wait(sema->cond, sema->mutex);
   }
   sema->counter--;
   pipe_mutex_unlock(sema->mutex);
}



/*
 * Thread-specific data.
 */

typedef struct {
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU) || defined(PIPE_OS_CYGWIN)
   pthread_key_t key;
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   DWORD key;
#endif
   int initMagic;
} pipe_tsd;


#define PIPE_TSD_INIT_MAGIC 0xff8adc98


static INLINE void
pipe_tsd_init(pipe_tsd *tsd)
{
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU) || defined(PIPE_OS_CYGWIN)
   if (pthread_key_create(&tsd->key, NULL/*free*/) != 0) {
      perror("pthread_key_create(): failed to allocate key for thread specific data");
      exit(-1);
   }
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   assert(0);
#endif
   tsd->initMagic = PIPE_TSD_INIT_MAGIC;
}

static INLINE void *
pipe_tsd_get(pipe_tsd *tsd)
{
   if (tsd->initMagic != (int) PIPE_TSD_INIT_MAGIC) {
      pipe_tsd_init(tsd);
   }
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU) || defined(PIPE_OS_CYGWIN)
   return pthread_getspecific(tsd->key);
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   assert(0);
   return NULL;
#else
   assert(0);
   return NULL;
#endif
}

static INLINE void
pipe_tsd_set(pipe_tsd *tsd, void *value)
{
   if (tsd->initMagic != (int) PIPE_TSD_INIT_MAGIC) {
      pipe_tsd_init(tsd);
   }
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU) || defined(PIPE_OS_CYGWIN)
   if (pthread_setspecific(tsd->key, value) != 0) {
      perror("pthread_set_specific() failed");
      exit(-1);
   }
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   assert(0);
#else
   assert(0);
#endif
}



#endif /* OS_THREAD_H_ */
