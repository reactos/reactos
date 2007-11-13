/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
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
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86$ */

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#ifndef __SIS_LOCK_H
#define __SIS_LOCK_H

extern void sisGetLock( sisContextPtr smesa, GLuint flags );

#ifdef DEBUG_LOCKING
extern char *prevLockFile;
extern int prevLockLine;
#define DEBUG_LOCK() \
  do { \
    prevLockFile=(__FILE__); \
    prevLockLine=(__LINE__); \
  } while (0)
#define DEBUG_RESET() \
  do { \
    prevLockFile=NULL; \
    prevLockLine=0; \
  } while (0)
#define DEBUG_CHECK_LOCK() \
  do { \
      if(prevLockFile){ \
        fprintf(stderr, "LOCK SET : %s:%d\n", __FILE__, __LINE__); \
      } \
  } while (0)
#else
#define DEBUG_LOCK()
#define DEBUG_RESET()
#define DEBUG_CHECK_LOCK()
#endif

/* Lock the hardware using the global current context */
#define LOCK_HARDWARE()							\
  do {									\
    char __ret=0;							\
    mEndPrimitive();							\
    DEBUG_CHECK_LOCK();							\
    DRM_CAS( smesa->driHwLock, smesa->hHWContext,			\
	     (DRM_LOCK_HELD | smesa->hHWContext), __ret );		\
    if ( __ret != 0 )							\
        sisGetLock( smesa, 0 );             					\
    DEBUG_LOCK();							\
  } while (0)

/* Unlock the hardware using the global current context */
#define UNLOCK_HARDWARE()						\
  do {									\
    mEndPrimitive(); 							\
    DRM_UNLOCK(smesa->driFd, smesa->driHwLock, 				\
	       smesa->hHWContext);					\
    DEBUG_RESET(); 							\
  } while (0)

#endif
