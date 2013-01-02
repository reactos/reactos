/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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


#include <stdio.h>
#include <stdlib.h>
#include "u_compiler.h"
#include "u_thread.h"


/*
 * This file should still compile even when THREADS is not defined.
 * This is to make things easier to deal with on the makefile scene..
 */
#ifdef THREADS
#include <errno.h>

/*
 * Error messages
 */
#define INIT_TSD_ERROR "_glthread_: failed to allocate key for thread specific data"
#define GET_TSD_ERROR "_glthread_: failed to get thread specific data"
#define SET_TSD_ERROR "_glthread_: thread failed to set thread specific data"


/*
 * Magic number to determine if a TSD object has been initialized.
 * Kind of a hack but there doesn't appear to be a better cross-platform
 * solution.
 */
#define INIT_MAGIC 0xff8adc98



/*
 * POSIX Threads -- The best way to go if your platform supports them.
 *                  Solaris >= 2.5 have POSIX threads, IRIX >= 6.4 reportedly
 *                  has them, and many of the free Unixes now have them.
 *                  Be sure to use appropriate -mt or -D_REENTRANT type
 *                  compile flags when building.
 */
#ifdef PTHREADS

unsigned long
u_thread_self(void)
{
   return (unsigned long) pthread_self();
}


void
u_tsd_init(struct u_tsd *tsd)
{
   if (pthread_key_create(&tsd->key, NULL/*free*/) != 0) {
      perror(INIT_TSD_ERROR);
      exit(-1);
   }
   tsd->initMagic = INIT_MAGIC;
}


void *
u_tsd_get(struct u_tsd *tsd)
{
   if (tsd->initMagic != (int) INIT_MAGIC) {
      u_tsd_init(tsd);
   }
   return pthread_getspecific(tsd->key);
}


void
u_tsd_set(struct u_tsd *tsd, void *ptr)
{
   if (tsd->initMagic != (int) INIT_MAGIC) {
      u_tsd_init(tsd);
   }
   if (pthread_setspecific(tsd->key, ptr) != 0) {
      perror(SET_TSD_ERROR);
      exit(-1);
   }
}

#endif /* PTHREADS */



/*
 * Win32 Threads.  The only available option for Windows 95/NT.
 * Be sure that you compile using the Multithreaded runtime, otherwise
 * bad things will happen.
 */
#ifdef WIN32

unsigned long
u_thread_self(void)
{
   return GetCurrentThreadId();
}


void
u_tsd_init(struct u_tsd *tsd)
{
   tsd->key = TlsAlloc();
   if (tsd->key == TLS_OUT_OF_INDEXES) {
      perror(INIT_TSD_ERROR);
      exit(-1);
   }
   tsd->initMagic = INIT_MAGIC;
}


void
u_tsd_destroy(struct u_tsd *tsd)
{
   if (tsd->initMagic != INIT_MAGIC) {
      return;
   }
   TlsFree(tsd->key);
   tsd->initMagic = 0x0;
}


void *
u_tsd_get(struct u_tsd *tsd)
{
   if (tsd->initMagic != INIT_MAGIC) {
      u_tsd_init(tsd);
   }
   return TlsGetValue(tsd->key);
}


void
u_tsd_set(struct u_tsd *tsd, void *ptr)
{
   /* the following code assumes that the struct u_tsd has been initialized
      to zero at creation */
   if (tsd->initMagic != INIT_MAGIC) {
      u_tsd_init(tsd);
   }
   if (TlsSetValue(tsd->key, ptr) == 0) {
      perror(SET_TSD_ERROR);
      exit(-1);
   }
}

#endif /* WIN32 */


#else  /* THREADS */


/*
 * no-op functions
 */

unsigned long
u_thread_self(void)
{
   return 0;
}


void
u_tsd_init(struct u_tsd *tsd)
{
   (void) tsd;
}


void *
u_tsd_get(struct u_tsd *tsd)
{
   (void) tsd;
   return NULL;
}


void
u_tsd_set(struct u_tsd *tsd, void *ptr)
{
   (void) tsd;
   (void) ptr;
}


#endif /* THREADS */
