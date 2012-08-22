/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 */


/*
 * Thread support for gl dispatch.
 *
 * Initial version by John Stone (j.stone@acm.org) (johns@cs.umr.edu)
 *                and Christoph Poliwoda (poliwoda@volumegraphics.com)
 * Revised by Keith Whitwell
 * Adapted for new gl dispatcher by Brian Paul
 * Modified for use in mapi by Chia-I Wu
 */

/*
 * If this file is accidentally included by a non-threaded build,
 * it should not cause the build to fail, or otherwise cause problems.
 * In general, it should only be included when needed however.
 */

#ifndef _U_THREAD_H_
#define _U_THREAD_H_

#include "u_compiler.h"

#if defined(PTHREADS)
#include <pthread.h> /* POSIX threads headers */
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#if defined(PTHREADS) || defined(_WIN32)
#ifndef THREADS
#define THREADS
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif


/*
 * POSIX threads. This should be your choice in the Unix world
 * whenever possible.  When building with POSIX threads, be sure
 * to enable any compiler flags which will cause the MT-safe
 * libc (if one exists) to be used when linking, as well as any
 * header macros for MT-safe errno, etc.  For Solaris, this is the -mt
 * compiler flag.  On Solaris with gcc, use -D_REENTRANT to enable
 * proper compiling for MT-safe libc etc.
 */
#if defined(PTHREADS)

struct u_tsd {
   pthread_key_t key;
   int initMagic;
};

typedef pthread_mutex_t u_mutex;

#define u_mutex_declare_static(name) \
   static u_mutex name = PTHREAD_MUTEX_INITIALIZER

#define u_mutex_init(name)    pthread_mutex_init(&(name), NULL)
#define u_mutex_destroy(name) pthread_mutex_destroy(&(name))
#define u_mutex_lock(name)    (void) pthread_mutex_lock(&(name))
#define u_mutex_unlock(name)  (void) pthread_mutex_unlock(&(name))

#endif /* PTHREADS */


/*
 * Windows threads. Should work with Windows NT and 95.
 * IMPORTANT: Link with multithreaded runtime library when THREADS are
 * used!
 */
#ifdef WIN32

struct u_tsd {
   DWORD key;
   int   initMagic;
};

typedef CRITICAL_SECTION u_mutex;

/* http://locklessinc.com/articles/pthreads_on_windows/ */
#define u_mutex_declare_static(name) \
   static u_mutex name = {(PCRITICAL_SECTION_DEBUG)-1, -1, 0, 0, 0, 0}

#define u_mutex_init(name)    InitializeCriticalSection(&name)
#define u_mutex_destroy(name) DeleteCriticalSection(&name)
#define u_mutex_lock(name)    EnterCriticalSection(&name)
#define u_mutex_unlock(name)  LeaveCriticalSection(&name)

#endif /* WIN32 */


/*
 * THREADS not defined
 */
#ifndef THREADS

struct u_tsd {
   int initMagic; 
};

typedef unsigned u_mutex;

#define u_mutex_declare_static(name)   static u_mutex name = 0
#define u_mutex_init(name)             (void) name
#define u_mutex_destroy(name)          (void) name
#define u_mutex_lock(name)             (void) name
#define u_mutex_unlock(name)           (void) name

#endif /* THREADS */


unsigned long
u_thread_self(void);

void
u_tsd_init(struct u_tsd *tsd);

void
u_tsd_destroy(struct u_tsd *tsd); /* WIN32 only */

void *
u_tsd_get(struct u_tsd *tsd);

void
u_tsd_set(struct u_tsd *tsd, void *ptr);


#ifdef __cplusplus
}
#endif

#endif /* _U_THREAD_H_ */
