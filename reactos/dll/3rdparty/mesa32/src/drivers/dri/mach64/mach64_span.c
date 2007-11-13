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
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	Jos�Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "mach64_context.h"
#include "mach64_ioctl.h"
#include "mach64_state.h"
#include "mach64_span.h"

#include "swrast/swrast.h"

#define DBG 0

#define LOCAL_VARS							\
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);			\
   __DRIscreenPrivate *sPriv = mmesa->driScreen;			\
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;			\
   driRenderbuffer *drb = (driRenderbuffer *) rb;			\
   GLuint height = dPriv->h;						\
   GLushort p;								\
   (void) p;

#define LOCAL_DEPTH_VARS						\
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);			\
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;			\
   __DRIscreenPrivate *driScreen = mmesa->driScreen;			\
   driRenderbuffer *drb = (driRenderbuffer *) rb;			\
   GLuint height = dPriv->h;						\
   char *buf = (char *)(driScreen->pFB + drb->offset +			\
			(dPriv->x + dPriv->y * drb->pitch) * 2)

#define LOCAL_STENCIL_VARS	LOCAL_DEPTH_VARS

#define Y_FLIP( _y )	(height - _y - 1)

#define HW_LOCK()

/* FIXME could/should we use dPriv->numClipRects like the other drivers? */
#define HW_CLIPLOOP()							\
   do {									\
      int _nc = mmesa->numClipRects;					\
									\
      while ( _nc-- ) {							\
	 int minx = mmesa->pClipRects[_nc].x1 - mmesa->drawX;		\
	 int miny = mmesa->pClipRects[_nc].y1 - mmesa->drawY;		\
	 int maxx = mmesa->pClipRects[_nc].x2 - mmesa->drawX;		\
	 int maxy = mmesa->pClipRects[_nc].y2 - mmesa->drawY;

#define HW_ENDCLIPLOOP()						\
      }									\
   } while (0)

#define HW_UNLOCK()



/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    mach64##x##_RGB565
#define TAG2(x,y) mach64##x##_RGB565##y
#define GET_PTR(X,Y) (sPriv->pFB + drb->offset		\
     + ((dPriv->y + (Y)) * drb->pitch + (dPriv->x + (X))) * drb->cpp)
#include "spantmp2.h"


/* 32 bit, ARGB8888 color spanline and pixel functions
 */
/* FIXME the old code always read back alpha as 0xff, i.e. fully opaque.
   Was there a reason to do so ? If so that'll won't work with that template... */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    mach64##x##_ARGB8888
#define TAG2(x,y) mach64##x##_ARGB8888##y
#define GET_PTR(X,Y) (sPriv->pFB + drb->offset		\
     + ((dPriv->y + (Y)) * drb->pitch + (dPriv->x + (X))) * drb->cpp)
#include "spantmp2.h"


/* ================================================================
 * Depth buffer
 */

/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)(buf + ((_x) + (_y) * drb->pitch) * 2) = d;

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)(buf + ((_x) + (_y) * drb->pitch) * 2);

#define TAG(x) mach64##x##_z16
#include "depthtmp.h"


static void mach64SpanRenderStart( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   LOCK_HARDWARE( mmesa );
   FINISH_DMA_LOCKED( mmesa );
}

static void mach64SpanRenderFinish( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   _swrast_flush( ctx );
   UNLOCK_HARDWARE( mmesa );
}

void mach64DDInitSpanFuncs( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart	= mach64SpanRenderStart;
   swdd->SpanRenderFinish	= mach64SpanRenderFinish;
}


/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
mach64SetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         mach64InitPointers_RGB565(&drb->Base);
      }
      else {
         mach64InitPointers_ARGB8888(&drb->Base);
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      mach64InitDepthPointers_z16(&drb->Base);
   }
}
