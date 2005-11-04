/**************************************************************************

Copyright 2001 VA Linux Systems Inc., Fremont, California.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_span.c,v 1.4 2002/12/10 01:26:53 dawes Exp $ */

/**
 * \file i830_span.c
 *
 * Heavily based on the I810 driver, which was written by Keith Whitwell.
 *
 * \author Jeff Hartmann <jhartmann@2d3d.com>
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "colormac.h"

#include "i830_screen.h"
#include "i830_dri.h"

#include "i830_span.h"
#include "i830_ioctl.h"
#include "swrast/swrast.h"


#define DBG 0

#define LOCAL_VARS						\
   i830ContextPtr imesa = I830_CONTEXT(ctx);                    \
   __DRIdrawablePrivate *dPriv = imesa->mesa_drawable;		\
   i830ScreenPrivate *i830Screen = imesa->i830Screen;		\
   GLuint pitch = i830Screen->backPitch * i830Screen->cpp;	\
   GLuint height = dPriv->h;					\
   char *buf = (char *)(imesa->drawMap +			\
			dPriv->x * i830Screen->cpp +		\
			dPriv->y * pitch);			\
   char *read_buf = (char *)(imesa->readMap +			\
			     dPriv->x * i830Screen->cpp +	\
			     dPriv->y * pitch); 		\
   GLushort p;         						\
   (void) read_buf; (void) buf; (void) p

#define LOCAL_DEPTH_VARS					\
   i830ContextPtr imesa = I830_CONTEXT(ctx);                    \
   __DRIdrawablePrivate *dPriv = imesa->mesa_drawable;		\
   i830ScreenPrivate *i830Screen = imesa->i830Screen;		\
   GLuint pitch = i830Screen->backPitch * i830Screen->cpp;	\
   GLuint height = dPriv->h;					\
   char *buf = (char *)(i830Screen->depth.map +			\
			dPriv->x * i830Screen->cpp +		\
			dPriv->y * pitch)

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS 

#define INIT_MONO_PIXEL(p,color)\
	 p = PACK_COLOR_565(color[0],color[1],color[2])

#define Y_FLIP(_y) (height - _y - 1)

#define HW_LOCK()

#define HW_UNLOCK()

/* 16 bit, 565 rgb color spanline and pixel functions
 */
#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = ( (((int)r & 0xf8) << 8) |	\
		                             (((int)g & 0xfc) << 3) |	\
		                             (((int)b & 0xf8) >> 3))
#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )				\
do {								\
   GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);	\
   rgba[0] = (((p >> 11) & 0x1f) * 255) / 31;			\
   rgba[1] = (((p >>  5) & 0x3f) * 255) / 63;			\
   rgba[2] = (((p >>  0) & 0x1f) * 255) / 31;			\
   rgba[3] = 255;						\
} while(0)

#define TAG(x) i830##x##_565
#include "spantmp.h"

/* 15 bit, 555 rgb color spanline and pixel functions
 */
#define WRITE_RGBA( _x, _y, r, g, b, a )			\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = (((r & 0xf8) << 7) |	\
		                            ((g & 0xf8) << 3) |	\
                         		    ((b & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch)  = p

#define READ_RGBA( rgba, _x, _y )				\
do {								\
   GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);	\
   rgba[0] = (p >> 7) & 0xf8;					\
   rgba[1] = (p >> 3) & 0xf8;					\
   rgba[2] = (p << 3) & 0xf8;					\
   rgba[3] = 255;						\
} while(0)

#define TAG(x) i830##x##_555
#include "spantmp.h"

/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d ) \
   *(GLushort *)(buf + _x*2 + _y*pitch)  = d;

#define READ_DEPTH( d, _x, _y )	\
   d = *(GLushort *)(buf + _x*2 + _y*pitch);	 


#define TAG(x) i830##x##_16
#include "depthtmp.h"


#undef LOCAL_VARS
#define LOCAL_VARS					\
   i830ContextPtr imesa = I830_CONTEXT(ctx);                    \
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;	\
   i830ScreenPrivate *i830Screen = imesa->i830Screen;	\
   GLuint pitch = i830Screen->backPitch * i830Screen->cpp;	\
   GLuint height = dPriv->h;				\
   char *buf = (char *)(imesa->drawMap +		\
			dPriv->x * i830Screen->cpp +			\
			dPriv->y * pitch);		\
   char *read_buf = (char *)(imesa->readMap +		\
			     dPriv->x * i830Screen->cpp +		\
			     dPriv->y * pitch); 	\
   GLuint p = I830_CONTEXT( ctx )->MonoColor;         \
   (void) read_buf; (void) buf; (void) p

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p,color)\
	 p = PACK_COLOR_888(color[0],color[1],color[2])

/* 32 bit, 8888 argb color spanline and pixel functions
 */
#define WRITE_RGBA(_x, _y, r, g, b, a)			\
    *(GLuint *)(buf + _x*4 + _y*pitch) = ((r << 16) |	\
					  (g << 8)  |	\
					  (b << 0)  |	\
					  (a << 24) )

#define WRITE_PIXEL(_x, _y, p)			\
    *(GLuint *)(buf + _x*4 + _y*pitch) = p


#define READ_RGBA(rgba, _x, _y)					\
    do {							\
	GLuint p = *(GLuint *)(read_buf + _x*4 + _y*pitch);	\
	rgba[0] = (p >> 16) & 0xff;				\
	rgba[1] = (p >> 8)  & 0xff;				\
	rgba[2] = (p >> 0)  & 0xff;				\
	rgba[3] = (p >> 24) & 0xff;				\
    } while (0)

#define TAG(x) i830##x##_8888
#include "spantmp.h"

/* 24 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLuint *)(buf + _x*4 + _y*pitch) = 0xffffff & d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + _x*4 + _y*pitch) & 0xffffff;

#define TAG(x) i830##x##_24
#include "depthtmp.h"

/* 24/8 bit interleaved depth/stencil functions
 */
#define WRITE_DEPTH( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   tmp &= 0xff000000;					\
   tmp |= (d) & 0xffffff;				\
   *(GLuint *)(buf + _x*4 + _y*pitch) = tmp;		\
}

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + _x*4 + _y*pitch) & 0xffffff;


#define TAG(x) i830##x##_24_8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   tmp &= 0xffffff;					\
   tmp |= (d<<24);					\
   *(GLuint *)(buf + _x*4 + _y*pitch) = tmp;		\
}

#define READ_STENCIL( d, _x, _y )			\
   d = *(GLuint *)(buf + _x*4 + _y*pitch) >> 24;

#define TAG(x) i830##x##_24_8
#include "stenciltmp.h"

/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void i830SetBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                          GLuint bufferBit)
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   
   assert( (colorBuffer == imesa->driDrawable->driverPrivate)
	   || (colorBuffer == imesa->driReadable->driverPrivate) );

   imesa->mesa_drawable = (colorBuffer == imesa->driDrawable->driverPrivate)
       ? imesa->driDrawable : imesa->driReadable;
   
   if (bufferBit == BUFFER_BIT_FRONT_LEFT) {
      imesa->drawMap = (char *)imesa->driScreen->pFB;
      imesa->readMap = (char *)imesa->driScreen->pFB;
   } else if (bufferBit == BUFFER_BIT_BACK_LEFT) {
      imesa->drawMap = imesa->i830Screen->back.map;
      imesa->readMap = imesa->i830Screen->back.map;
   } else {
      ASSERT(0);
   }
}



/* Move locking out to get reasonable span performance.
 */
void i830SpanRenderStart( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   I830_FIREVERTICES(imesa);
   LOCK_HARDWARE(imesa);
   i830RegetLockQuiescent( imesa );
}

void i830SpanRenderFinish( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   _swrast_flush( ctx );
   UNLOCK_HARDWARE( imesa );
}

void i830DDInitSpanFuncs( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   i830ScreenPrivate *i830Screen = imesa->i830Screen;

   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

   swdd->SetBuffer = i830SetBuffer;

   switch (i830Screen->fbFormat) {
   case DV_PF_555:
#if 0
      swdd->WriteRGBASpan = i830WriteRGBASpan_555;
      swdd->WriteRGBSpan = i830WriteRGBSpan_555;
      swdd->WriteMonoRGBASpan = i830WriteMonoRGBASpan_555;
      swdd->WriteRGBAPixels = i830WriteRGBAPixels_555;
      swdd->WriteMonoRGBAPixels = i830WriteMonoRGBAPixels_555;
      swdd->ReadRGBASpan = i830ReadRGBASpan_555;
      swdd->ReadRGBAPixels = i830ReadRGBAPixels_555;
      swdd->ReadDepthSpan = i830ReadDepthSpan_16;
      swdd->WriteDepthSpan = i830WriteDepthSpan_16;
      swdd->ReadDepthPixels = i830ReadDepthPixels_16;
      swdd->WriteDepthPixels = i830WriteDepthPixels_16;
#endif
      break;

   case DV_PF_565:
#if 0
      swdd->WriteRGBASpan = i830WriteRGBASpan_565;
      swdd->WriteRGBSpan = i830WriteRGBSpan_565;
      swdd->WriteMonoRGBASpan = i830WriteMonoRGBASpan_565;
      swdd->WriteRGBAPixels = i830WriteRGBAPixels_565;
      swdd->WriteMonoRGBAPixels = i830WriteMonoRGBAPixels_565; 
      swdd->ReadRGBASpan = i830ReadRGBASpan_565;
      swdd->ReadRGBAPixels = i830ReadRGBAPixels_565;
      swdd->ReadDepthSpan = i830ReadDepthSpan_16;
      swdd->WriteDepthSpan = i830WriteDepthSpan_16;
      swdd->ReadDepthPixels = i830ReadDepthPixels_16;
      swdd->WriteDepthPixels = i830WriteDepthPixels_16;
#endif
      break;

   case DV_PF_8888:
#if 0
      swdd->WriteRGBASpan = i830WriteRGBASpan_8888;
      swdd->WriteRGBSpan = i830WriteRGBSpan_8888;
      swdd->WriteMonoRGBASpan = i830WriteMonoRGBASpan_8888;
      swdd->WriteRGBAPixels = i830WriteRGBAPixels_8888;
      swdd->WriteMonoRGBAPixels = i830WriteMonoRGBAPixels_8888;
      swdd->ReadRGBASpan = i830ReadRGBASpan_8888;
      swdd->ReadRGBAPixels = i830ReadRGBAPixels_8888;
#endif

      if(imesa->hw_stencil) {
#if 0
	 swdd->ReadDepthSpan = i830ReadDepthSpan_24_8;
	 swdd->WriteDepthSpan = i830WriteDepthSpan_24_8;
	 swdd->ReadDepthPixels = i830ReadDepthPixels_24_8;
	 swdd->WriteDepthPixels = i830WriteDepthPixels_24_8;
	 swdd->WriteStencilSpan = i830WriteStencilSpan_24_8;
	 swdd->ReadStencilSpan = i830ReadStencilSpan_24_8;
	 swdd->WriteStencilPixels = i830WriteStencilPixels_24_8;
	 swdd->ReadStencilPixels = i830ReadStencilPixels_24_8;
#endif
      } else {
#if 0
	 swdd->ReadDepthSpan = i830ReadDepthSpan_24;
	 swdd->WriteDepthSpan = i830WriteDepthSpan_24;
	 swdd->ReadDepthPixels = i830ReadDepthPixels_24;
	 swdd->WriteDepthPixels = i830WriteDepthPixels_24;
#endif
      }
      break;
   }

   swdd->SpanRenderStart = i830SpanRenderStart;
   swdd->SpanRenderFinish = i830SpanRenderFinish; 
}


/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
i830SetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 5 && vis->blueBits == 5) {
         drb->Base.GetRow        = i830ReadRGBASpan_555;
         drb->Base.GetValues     = i830ReadRGBAPixels_555;
         drb->Base.PutRow        = i830WriteRGBASpan_555;
         drb->Base.PutRowRGB     = i830WriteRGBSpan_555;
         drb->Base.PutMonoRow    = i830WriteMonoRGBASpan_555;
         drb->Base.PutValues     = i830WriteRGBAPixels_555;
         drb->Base.PutMonoValues = i830WriteMonoRGBAPixels_555;
      }
      else if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         drb->Base.GetRow        = i830ReadRGBASpan_565;
         drb->Base.GetValues     = i830ReadRGBAPixels_565;
         drb->Base.PutRow        = i830WriteRGBASpan_565;
         drb->Base.PutRowRGB     = i830WriteRGBSpan_565;
         drb->Base.PutMonoRow    = i830WriteMonoRGBASpan_565;
         drb->Base.PutValues     = i830WriteRGBAPixels_565;
         drb->Base.PutMonoValues = i830WriteMonoRGBAPixels_565;
      }
      else {
         assert(vis->redBits == 8);
         assert(vis->greenBits == 8);
         assert(vis->blueBits == 8);
         drb->Base.GetRow        = i830ReadRGBASpan_8888;
         drb->Base.GetValues     = i830ReadRGBAPixels_8888;
         drb->Base.PutRow        = i830WriteRGBASpan_8888;
         drb->Base.PutRowRGB     = i830WriteRGBSpan_8888;
         drb->Base.PutMonoRow    = i830WriteMonoRGBASpan_8888;
         drb->Base.PutValues     = i830WriteRGBAPixels_8888;
         drb->Base.PutMonoValues = i830WriteMonoRGBAPixels_8888;
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      drb->Base.GetRow        = i830ReadDepthSpan_16;
      drb->Base.GetValues     = i830ReadDepthPixels_16;
      drb->Base.PutRow        = i830WriteDepthSpan_16;
      drb->Base.PutMonoRow    = i830WriteMonoDepthSpan_16;
      drb->Base.PutValues     = i830WriteDepthPixels_16;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      drb->Base.GetRow        = i830ReadDepthSpan_24_8;
      drb->Base.GetValues     = i830ReadDepthPixels_24_8;
      drb->Base.PutRow        = i830WriteDepthSpan_24_8;
      drb->Base.PutMonoRow    = i830WriteMonoDepthSpan_24_8;
      drb->Base.PutValues     = i830WriteDepthPixels_24_8;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT32) {
      /* not _really_ 32-bit Z */
      drb->Base.GetRow        = i830ReadDepthSpan_24;
      drb->Base.GetValues     = i830ReadDepthPixels_24;
      drb->Base.PutRow        = i830WriteDepthSpan_24;
      drb->Base.PutMonoRow    = i830WriteMonoDepthSpan_24;
      drb->Base.PutValues     = i830WriteDepthPixels_24;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      drb->Base.GetRow        = i830ReadStencilSpan_24_8;
      drb->Base.GetValues     = i830ReadStencilPixels_24_8;
      drb->Base.PutRow        = i830WriteStencilSpan_24_8;
      drb->Base.PutMonoRow    = i830WriteMonoStencilSpan_24_8;
      drb->Base.PutValues     = i830WriteStencilPixels_24_8;
      drb->Base.PutMonoValues = NULL;
   }
}
