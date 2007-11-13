/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_tris.h,v 1.8 2002/10/30 12:51:43 alanh Exp $ */
/**************************************************************************

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
ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 */

#ifndef __SIS_TRIS_H__
#define __SIS_TRIS_H__

#include "sis_lock.h"
#include "mtypes.h"

extern void sisInitTriFuncs( GLcontext *ctx );
extern void sisFlushPrims( sisContextPtr smesa );
extern void sisFlushPrimsLocked( sisContextPtr smesa );
extern void sisFallback( GLcontext *ctx, GLuint bit, GLboolean mode );

#define FALLBACK( smesa, bit, mode ) sisFallback( smesa->glCtx, bit, mode )

#define SIS_FIREVERTICES(smesa)				\
do {							\
   if (smesa->vb_cur != smesa->vb_last)			\
      sisFlushPrims(smesa);				\
} while (0)

static __inline GLuint *sisAllocDmaLow(sisContextPtr smesa, int bytes)
{
   GLuint *start;

   if (smesa->vb_cur + bytes >= smesa->vb_end) {
      LOCK_HARDWARE();
      sisFlushPrimsLocked(smesa);
      if (smesa->using_agp) {
	 WaitEngIdle(smesa);
	 smesa->vb_cur = smesa->vb;
	 smesa->vb_last = smesa->vb_cur;
      }
      UNLOCK_HARDWARE();
   }

   start = (GLuint *)smesa->vb_cur;
   smesa->vb_cur += bytes;
   return start;
}

#endif /* __SIS_TRIS_H__ */
