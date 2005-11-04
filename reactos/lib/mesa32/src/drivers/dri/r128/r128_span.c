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

#define LOCAL_VARS							\
   r128ContextPtr rmesa = R128_CONTEXT(ctx);				\
   r128ScreenPtr r128scrn = rmesa->r128Screen;				\
   __DRIscreenPrivate *sPriv = rmesa->driScreen;			\
   __DRIdrawablePrivate *dPriv = rmesa->driDrawable;			\
   GLuint pitch = r128scrn->frontPitch * r128scrn->cpp;			\
   GLuint height = dPriv->h;						\
   char *buf = (char *)(sPriv->pFB +					\
			rmesa->drawOffset +				\
			(dPriv->x * r128scrn->cpp) +			\
			(dPriv->y * pitch));				\
   char *read_buf = (char *)(sPriv->pFB +				\
			     rmesa->readOffset +			\
			     (dPriv->x * r128scrn->cpp) +		\
			     (dPriv->y * pitch));			\
   GLuint p;								\
   (void) read_buf; (void) buf; (void) p

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
#include "spantmp2.h"


/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    r128##x##_ARGB8888
#define TAG2(x,y) r128##x##_ARGB8888##y
#include "spantmp2.h"


/* ================================================================
 * Depth buffer
 */

/* 16-bit depth buffer functions
 */

#define WRITE_DEPTH_SPAN()						\
   r128WriteDepthSpanLocked( rmesa, n,					\
			     x + dPriv->x,				\
			     y + dPriv->y,				\
			     depth, mask );

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

#define TAG(x) r128##x##_16
#include "depthtmp.h"


/* 24-bit depth, 8-bit stencil buffer functions
 */
#define WRITE_DEPTH_SPAN()						\
   r128WriteDepthSpanLocked( rmesa, n,					\
			     x + dPriv->x,				\
			     y + dPriv->y,				\
			     depth, mask );

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
   GLuint *buf = (GLuint *)((GLubyte *)sPriv->pFB +			\
			    r128scrn->spanOffset);			\
   GLint i;								\
									\
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

#define TAG(x) r128##x##_24_8
#include "depthtmp.h"



/* ================================================================
 * Stencil buffer
 */

/* FIXME: Add support for hardware stencil buffers.
 */

/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void r128DDSetBuffer( GLcontext *ctx,
                             GLframebuffer *colorBuffer,
                             GLuint bufferBit )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   switch ( bufferBit ) {
   case BUFFER_BIT_FRONT_LEFT:
      if ( rmesa->sarea->pfCurrentPage == 1 ) {
         rmesa->drawOffset = rmesa->readOffset = rmesa->r128Screen->backOffset;
         rmesa->drawPitch  = rmesa->readPitch  = rmesa->r128Screen->backPitch;
      } else {
         rmesa->drawOffset = rmesa->readOffset = rmesa->r128Screen->frontOffset;
         rmesa->drawPitch  = rmesa->readPitch  = rmesa->r128Screen->frontPitch;
      }
      break;
   case BUFFER_BIT_BACK_LEFT:
      if ( rmesa->sarea->pfCurrentPage == 1 ) {
         rmesa->drawOffset = rmesa->readOffset = rmesa->r128Screen->frontOffset;
         rmesa->drawPitch  = rmesa->readPitch  = rmesa->r128Screen->frontPitch;
      } else {
         rmesa->drawOffset = rmesa->readOffset = rmesa->r128Screen->backOffset;
         rmesa->drawPitch  = rmesa->readPitch  = rmesa->r128Screen->backPitch;
      }
      break;
   default:
      break;
   }
}

void r128SpanRenderStart( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   FLUSH_BATCH(rmesa);
   LOCK_HARDWARE(rmesa);
   r128WaitForIdleLocked( rmesa );
}

void r128SpanRenderFinish( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   _swrast_flush( ctx );
   UNLOCK_HARDWARE( rmesa );
}

void r128DDInitSpanFuncs( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

   swdd->SetBuffer = r128DDSetBuffer;
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
      drb->Base.GetRow        = r128ReadDepthSpan_16;
      drb->Base.GetValues     = r128ReadDepthPixels_16;
      drb->Base.PutRow        = r128WriteDepthSpan_16;
      drb->Base.PutMonoRow    = r128WriteMonoDepthSpan_16;
      drb->Base.PutValues     = r128WriteDepthPixels_16;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      drb->Base.GetRow        = r128ReadDepthSpan_24_8;
      drb->Base.GetValues     = r128ReadDepthPixels_24_8;
      drb->Base.PutRow        = r128WriteDepthSpan_24_8;
      drb->Base.PutMonoRow    = r128WriteMonoDepthSpan_24_8;
      drb->Base.PutValues     = r128WriteDepthPixels_24_8;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      drb->Base.GetRow        = NULL;
      drb->Base.GetValues     = NULL;
      drb->Base.PutRow        = NULL;
      drb->Base.PutMonoRow    = NULL;
      drb->Base.PutValues     = NULL;
      drb->Base.PutMonoValues = NULL;
   }
}
