/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
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
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_lock.h,v 1.3 2002/02/22 21:45:03 dawes Exp $ */

/*
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __TDFX_LOCK_H__
#define __TDFX_LOCK_H__

/* You can turn this on to find locking conflicts.
 */
#define DEBUG_LOCKING		0

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

#endif /* DEBUG_LOCKING */


extern void tdfxGetLock( tdfxContextPtr fxMesa );


/* !!! We may want to separate locks from locks with validation.
   This could be used to improve performance for those things
   commands that do not do any drawing !!! */

#define DRM_LIGHT_LOCK_RETURN(fd,lock,context,__ret)                   \
	do {                                                           \
		DRM_CAS(lock,context,DRM_LOCK_HELD|context,__ret);     \
                if (__ret) drmGetLock(fd,context,0);                   \
        } while(0)

#define LOCK_HARDWARE( fxMesa )						\
   do {									\
      char __ret = 0;							\
									\
      DEBUG_CHECK_LOCK();						\
      DRM_CAS( fxMesa->driHwLock, fxMesa->hHWContext,			\
	      DRM_LOCK_HELD | fxMesa->hHWContext, __ret );		\
      if ( __ret ) {							\
	 tdfxGetLock( fxMesa );						\
      }									\
      DEBUG_LOCK();							\
   } while (0)

/* Unlock the hardware using the global current context */
#define UNLOCK_HARDWARE( fxMesa )					\
  do {									\
    DRM_UNLOCK( fxMesa->driFd, fxMesa->driHwLock, fxMesa->hHWContext );	\
    DEBUG_RESET();							\
  } while (0)

/*
 * This pair of macros makes a loop over the drawing operations
 * so it is not self contained and doesn't have the nice single
 * statement semantics of most macros.
 */
#define BEGIN_CLIP_LOOP(fxMesa)			\
  do {						\
    LOCK_HARDWARE( fxMesa );			\
    BEGIN_CLIP_LOOP_LOCKED( fxMesa )

#define BEGIN_CLIP_LOOP_LOCKED(fxMesa)				\
  do {								\
    int _nc = fxMesa->numClipRects;				\
    while (_nc--) {						\
      if (fxMesa->numClipRects > 1) {				\
        int _height = fxMesa->screen_height;			\
        fxMesa->Glide.grClipWindow(fxMesa->pClipRects[_nc].x1,	\
                     _height - fxMesa->pClipRects[_nc].y2,	\
                     fxMesa->pClipRects[_nc].x2,		\
                     _height - fxMesa->pClipRects[_nc].y1);	\
      }


#define END_CLIP_LOOP_LOCKED( fxMesa )		\
    }						\
  } while (0)

#define END_CLIP_LOOP( fxMesa )			\
    END_CLIP_LOOP_LOCKED( fxMesa );		\
    UNLOCK_HARDWARE( fxMesa );			\
  } while (0)

#endif /* __TDFX_LOCK_H__ */
