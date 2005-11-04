/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __RADEON_LOCK_H__
#define __RADEON_LOCK_H__

#include "r200_ioctl.h"

extern void radeonGetLock(radeonContextPtr radeon, GLuint flags);

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
#define LOCK_HARDWARE( radeon )						\
	do {								\
		char __ret = 0;						\
		DEBUG_CHECK_LOCK();					\
		DRM_CAS( (radeon)->dri.hwLock, (radeon)->dri.hwContext,	\
			(DRM_LOCK_HELD | (radeon)->dri.hwContext), __ret ); \
		if ( __ret )						\
			radeonGetLock( (radeon), 0 );			\
		DEBUG_LOCK();						\
	} while (0)

#if R200_MERGED
#define UNLOCK_HARDWARE( radeon )					\
	do {								\
		DRM_UNLOCK( (radeon)->dri.fd,				\
			(radeon)->dri.hwLock,				\
			(radeon)->dri.hwContext );			\
		DEBUG_RESET();						\
		if (IS_FAMILY_R200((radeon))) {				\
			r200ContextPtr __r200 = (r200ContextPtr)(radeon); \
			if (__r200->save_on_next_unlock)		\
				r200SaveHwState( __r200 );		\
			__r200->save_on_next_unlock = GL_FALSE;		\
		}							\
	} while (0)
#else
#define UNLOCK_HARDWARE( radeon )					\
	do {								\
		DRM_UNLOCK( (radeon)->dri.fd,				\
			(radeon)->dri.hwLock,				\
			(radeon)->dri.hwContext );			\
		DEBUG_RESET();						\
	} while (0)
#endif
#endif				/* __RADEON_LOCK_H__ */
