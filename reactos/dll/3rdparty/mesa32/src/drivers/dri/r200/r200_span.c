/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_span.c,v 1.1 2002/10/30 12:51:52 alanh Exp $ */
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

#include "glheader.h"
#include "imports.h"
#include "swrast/swrast.h"
#include "colormac.h"

#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_state.h"
#include "r200_span.h"
#include "r200_tex.h"

#define DBG 0

/*
 * Note that all information needed to access pixels in a renderbuffer
 * should be obtained through the gl_renderbuffer parameter, not per-context
 * information.
 */
#define LOCAL_VARS						\
   driRenderbuffer *drb = (driRenderbuffer *) rb;		\
   const __DRIdrawablePrivate *dPriv = drb->dPriv;		\
   const GLuint bottom = dPriv->h - 1;				\
   GLubyte *buf = (GLubyte *) drb->flippedData			\
      + (dPriv->y * drb->flippedPitch + dPriv->x) * drb->cpp;	\
   GLuint p;							\
   (void) p;

#define LOCAL_DEPTH_VARS				\
   driRenderbuffer *drb = (driRenderbuffer *) rb;	\
   const __DRIdrawablePrivate *dPriv = drb->dPriv;	\
   const GLuint bottom = dPriv->h - 1;			\
   GLuint xo = dPriv->x;				\
   GLuint yo = dPriv->y;				\
   GLubyte *buf = (GLubyte *) drb->Base.Data;

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS

#define Y_FLIP(Y) (bottom - (Y))

#define HW_LOCK() 

#define HW_UNLOCK()							



/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    r200##x##_RGB565
#define TAG2(x,y) r200##x##_RGB565##y
#define GET_PTR(X,Y) (buf + ((Y) * drb->flippedPitch + (X)) * 2)
#include "spantmp2.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    r200##x##_ARGB8888
#define TAG2(x,y) r200##x##_ARGB8888##y
#define GET_PTR(X,Y) (buf + ((Y) * drb->flippedPitch + (X)) * 4)
#include "spantmp2.h"


/* ================================================================
 * Depth buffer
 */

/* The Radeon family has depth tiling on all the time, so we have to convert
 * the x,y coordinates into the memory bus address (mba) in the same
 * manner as the engine.  In each case, the linear block address (ba)
 * is calculated, and then wired with x and y to produce the final
 * memory address.
 * The chip will do address translation on its own if the surface registers
 * are set up correctly. It is not quite enough to get it working with hyperz too...
 */

/* extract bit 'b' of x, result is zero or one */
#define BIT(x,b) ((x & (1<<b))>>b)

static GLuint
r200_mba_z32( driRenderbuffer *drb, GLint x, GLint y )
{
   GLuint pitch = drb->pitch;
   if (drb->depthHasSurface) {
      return 4 * (x + y * pitch);
   }
   else {
      GLuint b = ((y & 0x7FF) >> 4) * ((pitch & 0xFFF) >> 5) + ((x & 0x7FF) >> 5);
      GLuint a = 
         (BIT(x,0) << 2) |
         (BIT(y,0) << 3) |
         (BIT(x,1) << 4) |
         (BIT(y,1) << 5) |
         (BIT(x,3) << 6) |
         (BIT(x,4) << 7) |
         (BIT(x,2) << 8) |
         (BIT(y,2) << 9) |
         (BIT(y,3) << 10) |
         (((pitch & 0x20) ? (b & 0x01) : ((b & 0x01) ^ (BIT(y,4)))) << 11) |
         ((b >> 1) << 12);
      return a;
   }
}

static GLuint
r200_mba_z16( driRenderbuffer *drb, GLint x, GLint y )
{
   GLuint pitch = drb->pitch;
   if (drb->depthHasSurface) {
      return 2 * (x + y * pitch);
   }
   else {
      GLuint b = ((y & 0x7FF) >> 4) * ((pitch & 0xFFF) >> 6) + ((x & 0x7FF) >> 6);
      GLuint a = 
         (BIT(x,0) << 1) |
         (BIT(y,0) << 2) |
         (BIT(x,1) << 3) |
         (BIT(y,1) << 4) |
         (BIT(x,2) << 5) |
         (BIT(x,4) << 6) |
         (BIT(x,5) << 7) |
         (BIT(x,3) << 8) |
         (BIT(y,2) << 9) |
         (BIT(y,3) << 10) |
         (((pitch & 0x40) ? (b & 0x01) : ((b & 0x01) ^ (BIT(y,4)))) << 11) |
         ((b >> 1) << 12);
      return a;
   }
}


/* 16-bit depth buffer functions
 */

#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)(buf + r200_mba_z16( drb, _x + xo, _y + yo )) = d;

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)(buf + r200_mba_z16( drb, _x + xo, _y + yo ));

#define TAG(x) r200##x##_z16
#include "depthtmp.h"


/* 24 bit depth, 8 bit stencil depthbuffer functions
 */

#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint offset = r200_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xff000000;							\
   tmp |= ((d) & 0x00ffffff);						\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLuint *)(buf + r200_mba_z32( drb, _x + xo,			\
					 _y + yo )) & 0x00ffffff;

#define TAG(x) r200##x##_z24_s8
#include "depthtmp.h"


/* ================================================================
 * Stencil buffer
 */

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint offset = r200_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0x00ffffff;							\
   tmp |= (((d) & 0xff) << 24);						\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)

#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint offset = r200_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xff000000;							\
   d = tmp >> 24;							\
} while (0)

#define TAG(x) r200##x##_z24_s8
#include "stenciltmp.h"


/* Move locking out to get reasonable span performance (10x better
 * than doing this in HW_LOCK above).  WaitForIdle() is the main
 * culprit.
 */

static void r200SpanRenderStart( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );

   R200_FIREVERTICES( rmesa );
   LOCK_HARDWARE( rmesa );
   r200WaitForIdleLocked( rmesa );

   /* Read & rewrite the first pixel in the frame buffer.  This should
    * be a noop, right?  In fact without this conform fails as reading
    * from the framebuffer sometimes produces old results -- the
    * on-card read cache gets mixed up and doesn't notice that the
    * framebuffer has been updated.
    *
    * In the worst case this is buggy too as p might get the wrong
    * value first time, so really need a hidden pixel somewhere for this.
    */
   {
      int p;
      driRenderbuffer *drb =
	 (driRenderbuffer *) ctx->WinSysDrawBuffer->_ColorDrawBuffers[0][0];
      volatile int *buf =
	 (volatile int *)(rmesa->dri.screen->pFB + drb->offset);
      p = *buf;
      *buf = p;
   }
}

static void r200SpanRenderFinish( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   _swrast_flush( ctx );
   UNLOCK_HARDWARE( rmesa );
}

void r200InitSpanFuncs( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart          = r200SpanRenderStart;
   swdd->SpanRenderFinish         = r200SpanRenderFinish; 
}



/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
radeonSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         r200InitPointers_RGB565(&drb->Base);
      }
      else {
         r200InitPointers_ARGB8888(&drb->Base);
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      r200InitDepthPointers_z16(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      r200InitDepthPointers_z24_s8(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      r200InitStencilPointers_z24_s8(&drb->Base);
   }
}
