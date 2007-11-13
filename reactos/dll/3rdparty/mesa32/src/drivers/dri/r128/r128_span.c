/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_span.c,v 1.8 2002/10/30 12:51:39 alanh Exp $ */
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
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Kevin E. Martin <martin@valinux.com>
 *
 */

#include "r128_context.h"
#include "r128_ioctl.h"
#include "r128_state.h"
#include "r128_span.h"
#include "r128_tex.h"

#include "swrast/swrast.h"

#define DBG 0

#define HAVE_HW_DEPTH_SPANS	1
#define HAVE_HW_DEPTH_PIXELS	1
#define HAVE_HW_STENCIL_SPANS	1
#define HAVE_HW_STENCIL_PIXELS	1

#define LOCAL_VARS							\
   r128ContextPtr rmesa = R128_CONTEXT(ctx);				\
   __DRIscreenPrivate *sPriv = rmesa->driScreen;			\
   __DRIdrawablePrivate *dPriv = rmesa->driDrawable;			\
   driRenderbuffer *drb = (driRenderbuffer *) rb;			\
   GLuint height = dPriv->h;						\
   GLuint p;								\
   (void) p;

#define LOCAL_DEPTH_VARS						\
   r128ContextPtr rmesa = R128_CONTEXT(ctx);				\
   r128ScreenPtr r128scrn = rmesa->r128Screen;				\
   __DRIscreenPrivate *sPriv = rmesa->driScreen;			\
   __DRIdrawablePrivate *dPriv = rmesa->driDrawable;			\
   GLuint height = dPriv->h;						\
   (void) r128scrn; (void) sPriv; (void) height

#define LOCAL_STENCIL_VARS	LOCAL_DEPTH_VARS

#define Y_FLIP( _y )		(height - _y - 1)

#define HW_LOCK()

#define HW_UNLOCK()



/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    r128##x##_RGB565
#define TAG2(x,y) r128##x##_RGB565##y
#define GET_PTR(X,Y) (sPriv->pFB + drb->flippedOffset		\
     + ((dPriv->y + (Y)) * drb->flippedPitch + (dPriv->x + (X))) * drb->cpp)
#include "spantmp2.h"


/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    r128##x##_ARGB8888
#define TAG2(x,y) r128##x##_ARGB8888##y
#define GET_PTR(X,Y) (sPriv->pFB + drb->flippedOffset		\
     + ((dPriv->y + (Y)) * drb->flippedPitch + (dPriv->x + (X))) * drb->cpp)
#include "spantmp2.h"

/* Idling in the depth/stencil span functions:
 * For writes, the kernel reads from the given user-space buffer at dispatch
 * time, and then writes to the depth buffer asynchronously.
 * For reads, the kernel reads from the depth buffer and writes to the span
 * temporary asynchronously.
 * So, if we're going to read from the span temporary, we need to idle before
 * doing so.  But we don't need to idle after write, because the CPU won't
 * be accessing the destination, only the accelerator (through 3d rendering or
 * depth span reads)
 * However, due to interactions from pixel cache between 2d (what we do with
 * depth) and 3d (all other parts of the system), we idle at the begin and end
 * of a set of span operations, which should cover the pix cache issue.
 * Except, we still have major issues, as shown by no_rast=true glxgears, or
 * stencilwrap.
 */

/* ================================================================
 * Depth buffer
 */

/* These functions require locking */
#undef HW_LOCK
#undef HW_UNLOCK
#define HW_LOCK()    LOCK_HARDWARE(R128_CONTEXT(ctx));
#define HW_UNLOCK()  UNLOCK_HARDWARE(R128_CONTEXT(ctx));

/* 16-bit depth buffer functions
 */

#define WRITE_DEPTH_SPAN()						\
do {									\
   r128WriteDepthSpanLocked( rmesa, n,					\
			     x + dPriv->x,				\
			     y + dPriv->y,				\
			     depth, mask );				\
} while (0)

#define WRITE_DEPTH_PIXELS()						\
do {									\
   GLint ox[MAX_WIDTH];							\
   GLint oy[MAX_WIDTH];							\
   for ( i = 0 ; i < n ; i++ ) {					\
      ox[i] = x[i] + dPriv->x;						\
      oy[i] = Y_FLIP( y[i] ) + dPriv->y;				\
   }									\
   r128WriteDepthPixelsLocked( rmesa, n, ox, oy, depth, mask );		\
} while (0)

#define READ_DEPTH_SPAN()						\
do {									\
   GLushort *buf = (GLushort *)((GLubyte *)sPriv->pFB +			\
				r128scrn->spanOffset);			\
   GLint i;								\
									\
   r128ReadDepthSpanLocked( rmesa, n,					\
			    x + dPriv->x,				\
			    y + dPriv->y );				\
   r128WaitForIdleLocked( rmesa );					\
									\
   for ( i = 0 ; i < n ; i++ ) {					\
      depth[i] = buf[i];						\
   }									\
} while (0)

#define READ_DEPTH_PIXELS()						\
do {									\
   GLushort *buf = (GLushort *)((GLubyte *)sPriv->pFB +			\
				r128scrn->spanOffset);			\
   GLint i, remaining = n;						\
									\
   while ( remaining > 0 ) {						\
      GLint ox[128];							\
      GLint oy[128];							\
      GLint count;							\
									\
      if ( remaining <= 128 ) {						\
	 count = remaining;						\
      } else {								\
	 count = 128;							\
      }									\
      for ( i = 0 ; i < count ; i++ ) {					\
	 ox[i] = x[i] + dPriv->x;					\
	 oy[i] = Y_FLIP( y[i] ) + dPriv->y;				\
      }									\
									\
      r128ReadDepthPixelsLocked( rmesa, count, ox, oy );		\
      r128WaitForIdleLocked( rmesa );					\
									\
      for ( i = 0 ; i < count ; i++ ) {					\
	 depth[i] = buf[i];						\
      }									\
      depth += count;							\
      x += count;							\
      y += count;							\
      remaining -= count;						\
   }									\
} while (0)

#define TAG(x) r128##x##_z16
#include "depthtmp.h"


/* 24-bit depth, 8-bit stencil buffer functions
 */
#define WRITE_DEPTH_SPAN()						\
do {									\
   GLuint buf[n];							\
   GLint i;								\
   GLuint *readbuf = (GLuint *)((GLubyte *)sPriv->pFB +			\
				r128scrn->spanOffset);			\
   r128ReadDepthSpanLocked( rmesa, n,					\
			    x + dPriv->x,				\
			    y + dPriv->y );				\
   r128WaitForIdleLocked( rmesa );					\
   for ( i = 0 ; i < n ; i++ ) {					\
      buf[i] = (readbuf[i] & 0xff000000) | (depth[i] & 0x00ffffff);	\
   }									\
   r128WriteDepthSpanLocked( rmesa, n,					\
			     x + dPriv->x,				\
			     y + dPriv->y,				\
			     buf, mask );				\
} while (0)

#define WRITE_DEPTH_PIXELS()						\
do {									\
   GLuint buf[n];							\
   GLint ox[MAX_WIDTH];							\
   GLint oy[MAX_WIDTH];							\
   GLuint *readbuf = (GLuint *)((GLubyte *)sPriv->pFB +			\
				r128scrn->spanOffset);			\
   for ( i = 0 ; i < n ; i++ ) {					\
      ox[i] = x[i] + dPriv->x;						\
      oy[i] = Y_FLIP( y[i] ) + dPriv->y;				\
   }									\
   r128ReadDepthPixelsLocked( rmesa, n, ox, oy );			\
   r128WaitForIdleLocked( rmesa );					\
   for ( i = 0 ; i < n ; i++ ) {					\
      buf[i] = (readbuf[i] & 0xff000000) | (depth[i] & 0x00ffffff);	\
   }									\
   r128WriteDepthPixelsLocked( rmesa, n, ox, oy, buf, mask );		\
} while (0)

#define READ_DEPTH_SPAN()						\
do {									\
   GLuint *buf = (GLuint *)((GLubyte *)sPriv->pFB +			\
			    r128scrn->spanOffset);			\
   GLint i;								\
									\
   /*if (n >= 128) fprintf(stderr, "Large number of pixels: %d\n", n);*/	\
   r128ReadDepthSpanLocked( rmesa, n,					\
			    x + dPriv->x,				\
			    y + dPriv->y );				\
   r128WaitForIdleLocked( rmesa );					\
									\
   for ( i = 0 ; i < n ; i++ ) {					\
      depth[i] = buf[i] & 0x00ffffff;					\
   }									\
} while (0)

#define READ_DEPTH_PIXELS()						\
do {									\
   GLuint *buf = (GLuint *)((GLubyte *)sPriv->pFB +			\
			    r128scrn->spanOffset);			\
   GLint i, remaining = n;						\
									\
   while ( remaining > 0 ) {						\
      GLint ox[128];							\
      GLint oy[128];							\
      GLint count;							\
									\
      if ( remaining <= 128 ) {						\
	 count = remaining;						\
      } else {								\
	 count = 128;							\
      }									\
      for ( i = 0 ; i < count ; i++ ) {					\
	 ox[i] = x[i] + dPriv->x;					\
	 oy[i] = Y_FLIP( y[i] ) + dPriv->y;				\
      }									\
									\
      r128ReadDepthPixelsLocked( rmesa, count, ox, oy );		\
      r128WaitForIdleLocked( rmesa );					\
									\
      for ( i = 0 ; i < count ; i++ ) {					\
	 depth[i] = buf[i] & 0x00ffffff;				\
      }									\
      depth += count;							\
      x += count;							\
      y += count;							\
      remaining -= count;						\
   }									\
} while (0)

#define TAG(x) r128##x##_z24_s8
#include "depthtmp.h"



/* ================================================================
 * Stencil buffer
 */

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#define WRITE_STENCIL_SPAN()						\
do {									\
   GLuint buf[n];							\
   GLint i;								\
   GLuint *readbuf = (GLuint *)((GLubyte *)sPriv->pFB +			\
				r128scrn->spanOffset);			\
   r128ReadDepthSpanLocked( rmesa, n,					\
			    x + dPriv->x,				\
			    y + dPriv->y );				\
   r128WaitForIdleLocked( rmesa );					\
   for ( i = 0 ; i < n ; i++ ) {					\
      buf[i] = (readbuf[i] & 0x00ffffff) | (stencil[i] << 24);		\
   }									\
   r128WriteDepthSpanLocked( rmesa, n,					\
			     x + dPriv->x,				\
			     y + dPriv->y,				\
			     buf, mask );				\
} while (0)

#define WRITE_STENCIL_PIXELS()						\
do {									\
   GLuint buf[n];							\
   GLint ox[MAX_WIDTH];							\
   GLint oy[MAX_WIDTH];							\
   GLuint *readbuf = (GLuint *)((GLubyte *)sPriv->pFB +			\
				r128scrn->spanOffset);			\
   for ( i = 0 ; i < n ; i++ ) {					\
      ox[i] = x[i] + dPriv->x;						\
      oy[i] = Y_FLIP( y[i] ) + dPriv->y;				\
   }									\
   r128ReadDepthPixelsLocked( rmesa, n, ox, oy );			\
   r128WaitForIdleLocked( rmesa );					\
   for ( i = 0 ; i < n ; i++ ) {					\
      buf[i] = (readbuf[i] & 0x00ffffff) | (stencil[i] << 24);		\
   }									\
   r128WriteDepthPixelsLocked( rmesa, n, ox, oy, buf, mask );		\
} while (0)

#define READ_STENCIL_SPAN()						\
do {									\
   GLuint *buf = (GLuint *)((GLubyte *)sPriv->pFB +			\
			    r128scrn->spanOffset);			\
   GLint i;								\
									\
   /*if (n >= 128) fprintf(stderr, "Large number of pixels: %d\n", n);*/	\
   r128ReadDepthSpanLocked( rmesa, n,					\
			    x + dPriv->x,				\
			    y + dPriv->y );				\
   r128WaitForIdleLocked( rmesa );					\
									\
   for ( i = 0 ; i < n ; i++ ) {					\
      stencil[i] = (buf[i] & 0xff000000) >> 24;				\
   }									\
} while (0)

#define READ_STENCIL_PIXELS()						\
do {									\
   GLuint *buf = (GLuint *)((GLubyte *)sPriv->pFB +			\
			    r128scrn->spanOffset);			\
   GLint i, remaining = n;						\
									\
   while ( remaining > 0 ) {						\
      GLint ox[128];							\
      GLint oy[128];							\
      GLint count;							\
									\
      if ( remaining <= 128 ) {						\
	 count = remaining;						\
      } else {								\
	 count = 128;							\
      }									\
      for ( i = 0 ; i < count ; i++ ) {					\
	 ox[i] = x[i] + dPriv->x;					\
	 oy[i] = Y_FLIP( y[i] ) + dPriv->y;				\
      }									\
									\
      r128ReadDepthPixelsLocked( rmesa, count, ox, oy );		\
      r128WaitForIdleLocked( rmesa );					\
									\
      for ( i = 0 ; i < count ; i++ ) {					\
	 stencil[i] = (buf[i] & 0xff000000) >> 24;			\
      }									\
      stencil += count;							\
      x += count;							\
      y += count;							\
      remaining -= count;						\
   }									\
} while (0)

#define TAG(x) radeon##x##_z24_s8
#include "stenciltmp.h"

static void
r128SpanRenderStart( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   FLUSH_BATCH(rmesa);
   LOCK_HARDWARE(rmesa);
   r128WaitForIdleLocked( rmesa );
}

static void
r128SpanRenderFinish( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   _swrast_flush( ctx );
   r128WaitForIdleLocked( rmesa );
   UNLOCK_HARDWARE( rmesa );
}

void r128DDInitSpanFuncs( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart	= r128SpanRenderStart;
   swdd->SpanRenderFinish	= r128SpanRenderFinish;
}


/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
r128SetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         r128InitPointers_RGB565(&drb->Base);
      }
      else {
         r128InitPointers_ARGB8888(&drb->Base);
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      r128InitDepthPointers_z16(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      r128InitDepthPointers_z24_s8(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      radeonInitStencilPointers_z24_s8(&drb->Base);
   }
}
