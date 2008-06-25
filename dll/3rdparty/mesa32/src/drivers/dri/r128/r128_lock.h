/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_lock.h,v 1.4 2001/01/08 01:07:21 martin Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __R128_LOCK_H__
#define __R128_LOCK_H__

extern void r128GetLock( r128ContextPtr rmesa, GLuint flags );

/* Turn DEBUG_LOCKING on to find locking conflicts.
 */
#define DEBUG_LOCKING	0

#if DEBUG_LOCKING
extern char *prevLockFile;
extern int prevLockLine;

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
#define LOCK_HARDWARE( rmesa )						\
   do {									\
      char __ret = 0;							\
      DEBUG_CHECK_LOCK();						\
      DRM_CAS( rmesa->driHwLock, rmesa->hHWContext,			\
	       (DRM_LOCK_HELD | rmesa->hHWContext), __ret );		\
      if ( __ret )							\
	 r128GetLock( rmesa, 0 );					\
      DEBUG_LOCK();							\
   } while (0)

/* Unlock the hardware.
 */
#define UNLOCK_HARDWARE( rmesa )					\
   do {									\
      DRM_UNLOCK( rmesa->driFd,						\
		  rmesa->driHwLock,					\
		  rmesa->hHWContext );					\
      DEBUG_RESET();							\
   } while (0)

#endif /* __R128_LOCK_H__ */
