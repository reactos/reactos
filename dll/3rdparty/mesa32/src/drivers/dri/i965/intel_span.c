/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "colormac.h"

#include "intel_screen.h"
#include "intel_regions.h"
#include "intel_span.h"
#include "intel_ioctl.h"
#include "intel_tex.h"
#include "intel_batchbuffer.h"
#include "swrast/swrast.h"

#undef DBG
#define DBG 0

#define LOCAL_VARS						\
   struct intel_context *intel = intel_context(ctx);                    \
   __DRIdrawablePrivate *dPriv = intel->driDrawable;		\
   driRenderbuffer *drb = (driRenderbuffer *) rb;		\
   GLuint pitch = drb->pitch;					\
   GLuint height = dPriv->h;					\
   char *buf = (char *) drb->Base.Data +			\
			dPriv->x * drb->cpp +			\
			dPriv->y * pitch;			\
   GLushort p;							\
   (void) buf; (void) p

#define LOCAL_DEPTH_VARS					\
   struct intel_context *intel = intel_context(ctx);                    \
   __DRIdrawablePrivate *dPriv = intel->driDrawable;		\
   driRenderbuffer *drb = (driRenderbuffer *) rb;		\
   GLuint pitch = drb->pitch;					\
   GLuint height = dPriv->h;					\
   char *buf = (char *) drb->Base.Data +			\
			dPriv->x * drb->cpp +			\
			dPriv->y * pitch

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS 

#define INIT_MONO_PIXEL(p,color)\
	 p = INTEL_PACKCOLOR565(color[0],color[1],color[2])

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
   GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);		\
   rgba[0] = (((p >> 11) & 0x1f) * 255) / 31;			\
   rgba[1] = (((p >>  5) & 0x3f) * 255) / 63;			\
   rgba[2] = (((p >>  0) & 0x1f) * 255) / 31;			\
   rgba[3] = 255;						\
} while(0)

#define TAG(x) intel##x##_565
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
   GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);		\
   rgba[0] = (p >> 7) & 0xf8;					\
   rgba[1] = (p >> 3) & 0xf8;					\
   rgba[2] = (p << 3) & 0xf8;					\
   rgba[3] = 255;						\
} while(0)

#define TAG(x) intel##x##_555
#include "spantmp.h"

/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d ) \
   *(GLushort *)(buf + (_x)*2 + (_y)*pitch)  = d;

#define READ_DEPTH( d, _x, _y )	\
   d = *(GLushort *)(buf + (_x)*2 + (_y)*pitch);	 


#define TAG(x) intel##x##_z16
#include "depthtmp.h"


#undef LOCAL_VARS
#define LOCAL_VARS						\
   struct intel_context *intel = intel_context(ctx);			\
   __DRIdrawablePrivate *dPriv = intel->driDrawable;		\
   driRenderbuffer *drb = (driRenderbuffer *) rb;		\
   GLuint pitch = drb->pitch;					\
   GLuint height = dPriv->h;					\
   char *buf = (char *)drb->Base.Data +				\
			dPriv->x * drb->cpp +			\
			dPriv->y * pitch;			\
   GLuint p;							\
   (void) buf; (void) p

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p,color)\
	 p = INTEL_PACKCOLOR8888(color[0],color[1],color[2],color[3])

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
	GLuint p = *(GLuint *)(buf + _x*4 + _y*pitch);		\
	rgba[0] = (p >> 16) & 0xff;				\
	rgba[1] = (p >> 8)  & 0xff;				\
	rgba[2] = (p >> 0)  & 0xff;				\
	rgba[3] = (p >> 24) & 0xff;				\
    } while (0)

#define TAG(x) intel##x##_8888
#include "spantmp.h"


/* 24/8 bit interleaved depth/stencil functions
 */
#define WRITE_DEPTH( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*pitch);	\
   tmp &= 0xff000000;					\
   tmp |= (d) & 0xffffff;				\
   *(GLuint *)(buf + (_x)*4 + (_y)*pitch) = tmp;		\
}

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*pitch) & 0xffffff;


#define TAG(x) intel##x##_z24_s8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*pitch);	\
   tmp &= 0xffffff;					\
   tmp |= ((d)<<24);					\
   *(GLuint *)(buf + (_x)*4 + (_y)*pitch) = tmp;		\
}

#define READ_STENCIL( d, _x, _y )			\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*pitch) >> 24;

#define TAG(x) intel##x##_z24_s8
#include "stenciltmp.h"


/* Move locking out to get reasonable span performance.
 */
void intelSpanRenderStart( GLcontext *ctx )
{
   struct intel_context *intel = intel_context(ctx);

   if (intel->need_flush) {
      LOCK_HARDWARE(intel);
      intel->vtbl.emit_flush(intel, 0);
      intel_batchbuffer_flush(intel->batch);
      intel->need_flush = 0;
      UNLOCK_HARDWARE(intel);
      intelFinish(&intel->ctx);
   }


   LOCK_HARDWARE(intel);

   /* Just map the framebuffer and all textures.  Bufmgr code will
    * take care of waiting on the necessary fences:
    */
   intel_region_map(intel, intel->front_region);
   intel_region_map(intel, intel->back_region);
   intel_region_map(intel, intel->depth_region);
}

void intelSpanRenderFinish( GLcontext *ctx )
{
   struct intel_context *intel = intel_context( ctx );

   _swrast_flush( ctx );

   /* Now unmap the framebuffer:
    */
   intel_region_unmap(intel, intel->front_region);
   intel_region_unmap(intel, intel->back_region);
   intel_region_unmap(intel, intel->depth_region);

   UNLOCK_HARDWARE( intel );
}

void intelInitSpanFuncs( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart = intelSpanRenderStart;
   swdd->SpanRenderFinish = intelSpanRenderFinish; 
}


/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
intelSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 5 && vis->blueBits == 5) {
         intelInitPointers_555(&drb->Base);
      }
      else if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         intelInitPointers_565(&drb->Base);
      }
      else {
         assert(vis->redBits == 8);
         assert(vis->greenBits == 8);
         assert(vis->blueBits == 8);
         intelInitPointers_8888(&drb->Base);
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      intelInitDepthPointers_z16(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      intelInitDepthPointers_z24_s8(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      intelInitStencilPointers_z24_s8(&drb->Base);
   }
}
