/**************************************************************************

Copyright 2006 Stephane Marchesin
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


#ifndef __NOUVEAU_LOCK_H__
#define __NOUVEAU_LOCK_H__

#include "nouveau_context.h"

extern void nouveauGetLock( nouveauContextPtr nmesa, GLuint flags );

/*
 * !!! We may want to separate locks from locks with validation.  This
 * could be used to improve performance for those things commands that
 * do not do any drawing !!!
 */

/* Lock the hardware and validate our state.
 */
#define LOCK_HARDWARE( nmesa )						\
   do {									\
      char __ret = 0;							\
      DEBUG_CHECK_LOCK();						\
      DRM_CAS( nmesa->driHwLock, nmesa->hHWContext,			\
	       (DRM_LOCK_HELD | nmesa->hHWContext), __ret );		\
      if ( __ret )							\
	 nouveauGetLock( nmesa, 0 );					\
      DEBUG_LOCK();							\
   } while (0)

/* Unlock the hardware.
 */
#define UNLOCK_HARDWARE( nmesa )					\
   do {									\
      DRM_UNLOCK( nmesa->driFd,						\
		  nmesa->driHwLock,					\
		  nmesa->hHWContext );					\
      DEBUG_RESET();							\
   } while (0)

#define DEBUG_LOCK()
#define DEBUG_RESET()
#define DEBUG_CHECK_LOCK()


#endif /* __NOUVEAU_LOCK_H__ */
