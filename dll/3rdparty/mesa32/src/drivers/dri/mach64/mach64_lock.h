/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __MACH64_LOCK_H__
#define __MACH64_LOCK_H__

extern void mach64GetLock( mach64ContextPtr mmesa, GLuint flags );


/* Turn DEBUG_LOCKING on to find locking conflicts.
 */
#define DEBUG_LOCKING	1

#if DEBUG_LOCKING
extern char *prevLockFile;
extern int   prevLockLine;

#define DEBUG_LOCK()							\
   do {									\
      prevLockFile = (__FILE__);					\
      prevLockLine = (__LINE__);					\
   } while (0)

#define DEBUG_RESET()							\
   do {									\
      prevLockFile = 0;							\
      prevLockLine = 0;							\
   } while (0)

#define DEBUG_CHECK_LOCK()						\
   do {									\
      if ( prevLockFile ) {						\
	 fprintf( stderr,						\
		  "LOCK SET!\n\tPrevious %s:%d\n\tCurrent: %s:%d\n",	\
		  prevLockFile, prevLockLine, __FILE__, __LINE__ );	\
	 exit( 1 );							\
      }									\
   } while (0)

#else

#define DEBUG_LOCK()
#define DEBUG_RESET()
#define DEBUG_CHECK_LOCK()

#endif

/*
 * !!! We may want to separate locks from locks with validation.  This
 * could be used to improve performance for those things commands that
 * do not do any drawing !!!
 */

/* Lock the hardware and validate our state.
 */
#define LOCK_HARDWARE( mmesa )						\
   do {									\
      char __ret = 0;							\
      DEBUG_CHECK_LOCK();						\
      DRM_CAS( mmesa->driHwLock, mmesa->hHWContext,			\
	       (DRM_LOCK_HELD | mmesa->hHWContext), __ret );		\
      if ( __ret )							\
	 mach64GetLock( mmesa, 0 );					\
      DEBUG_LOCK();							\
   } while (0)

/* Unlock the hardware.
 */
#define UNLOCK_HARDWARE( mmesa )					\
   do {									\
      DRM_UNLOCK( mmesa->driFd,						\
		  mmesa->driHwLock,					\
		  mmesa->hHWContext );					\
      DEBUG_RESET();							\
   } while (0)

#endif /* __MACH64_LOCK_H__ */
