/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "mtypes.h"
#include "savagedd.h"
#include "savagespan.h"
#include "savageioctl.h"
#include "savage_bci.h"
#include "savage_3d_reg.h"
#include "swrast/swrast.h"

#define DBG 0

#define LOCAL_VARS						\
   driRenderbuffer *drb = (driRenderbuffer *) rb;		\
   __DRIdrawablePrivate *const dPriv = drb->dPriv;		\
   GLuint cpp   = drb->cpp;					\
   GLuint pitch = drb->pitch;					\
   GLuint height = dPriv->h;					\
   GLubyte *buf = drb->Base.Data + dPriv->x * cpp + dPriv->y * pitch;	\
   GLuint p;							\
   (void) p

#define LOCAL_DEPTH_VARS					\
   driRenderbuffer *drb = (driRenderbuffer *) rb;		\
   __DRIdrawablePrivate *const dPriv = drb->dPriv;		\
   GLuint zpp   = drb->cpp;					\
   GLuint pitch = drb->pitch;					\
   GLuint height = dPriv->h;					\
   GLubyte *buf = drb->Base.Data + dPriv->x * zpp + dPriv->y * pitch;

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS

#define Y_FLIP(_y) (height - _y - 1)

#define HW_LOCK()

#define HW_UNLOCK()

#define HW_WRITE_LOCK()

#define HW_READ_LOCK()


/* 16 bit, 565 rgb color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x) savage##x##_565
#define TAG2(x,y) savage##x##_565##y
#include "spantmp2.h"


/* 32 bit, 8888 ARGB color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x) savage##x##_8888
#define TAG2(x,y) savage##x##_8888##y
#include "spantmp2.h"


#undef HW_WRITE_LOCK
#define HW_WRITE_LOCK()
#undef HW_READ_LOCK
#define HW_READ_LOCK()



/* 16 bit integer depthbuffer functions
 * Depth range is reversed. See also savageCalcViewport.
 */
#define WRITE_DEPTH( _x, _y, d ) \
    *(GLushort *)(buf + ((_x)<<1) + (_y)*pitch) = 0xFFFF - d

#define READ_DEPTH( d, _x, _y ) \
    d = 0xFFFF - *(GLushort *)(buf + ((_x)<<1) + (_y)*pitch)

#define TAG(x) savage##x##_z16
#include "depthtmp.h"




/* 16 bit float depthbuffer functions
 */
#define WRITE_DEPTH( _x, _y, d ) \
    *(GLushort *)(buf + ((_x)<<1) + (_y)*pitch) = \
        savageEncodeFloat16( 1.0 - (GLfloat)d/65535.0 )

#define READ_DEPTH( d, _x, _y ) \
    d = 65535 - \
        savageDecodeFloat16( *(GLushort *)(buf + ((_x)<<1) + (_y)*pitch) ) * \
	65535.0

#define TAG(x) savage##x##_z16f
#include "depthtmp.h"




/* 8-bit stencil /24-bit integer depth depthbuffer functions.
 * Depth range is reversed. See also savageCalcViewport.
 */
#define WRITE_DEPTH( _x, _y, d ) do {				\
   GLuint tmp = *(GLuint *)(buf + ((_x)<<2) + (_y)*pitch);	\
   tmp &= 0xFF000000;						\
   tmp |= 0x00FFFFFF - d;					\
   *(GLuint *)(buf + (_x<<2) + _y*pitch)  = tmp;		\
} while(0)

#define READ_DEPTH( d, _x, _y )	\
   d = 0x00FFFFFF - (*(GLuint *)(buf + ((_x)<<2) + (_y)*pitch) & 0x00FFFFFF)

#define TAG(x) savage##x##_s8_z24
#include "depthtmp.h"




/* 24 bit float depthbuffer functions
 */
#define WRITE_DEPTH( _x, _y, d ) do {				\
    GLuint tmp = *(GLuint *)(buf + ((_x)<<2) + (_y)*pitch);	\
    tmp &= 0xFF000000;						\
    tmp |= savageEncodeFloat24( 1.0 - (GLfloat)d/16777215.0 );	\
   *(GLuint *)(buf + (_x<<2) + _y*pitch)  = tmp;		\
} while(0)

#define READ_DEPTH( d, _x, _y )					\
    d = 16777215 - savageDecodeFloat24(				\
	*(GLuint *)(buf + ((_x)<<2) + (_y)*pitch) & 0x00FFFFFF)	\
	* 16777215.0

#define TAG(x) savage##x##_s8_z24f
#include "depthtmp.h"


#define WRITE_STENCIL( _x, _y, d ) do {				\
   GLuint tmp = *(GLuint *)(buf + ((_x)<<2) + (_y)*pitch);	\
   tmp &= 0x00FFFFFF;						\
   tmp |= (((GLuint)d)<<24) & 0xFF000000;			\
   *(GLuint *)(buf + ((_x)<<2) + (_y)*pitch) = tmp;		\
} while(0)

#define READ_STENCIL( d, _x, _y ) \
   d = (GLstencil)((*(GLuint *)(buf + ((_x)<<2) + (_y)*pitch) & 0xFF000000) >> 24)

#define TAG(x) savage##x##_s8_z24
#include "stenciltmp.h"



/*
 * Wrappers around _swrast_Copy/Draw/ReadPixels that make sure all
 * primitives are flushed and the hardware is idle before accessing
 * the frame buffer.
 */
static void
savageCopyPixels( GLcontext *ctx,
		  GLint srcx, GLint srcy, GLsizei width, GLsizei height,
		  GLint destx, GLint desty,
		  GLenum type )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    FLUSH_BATCH(imesa);
    WAIT_IDLE_EMPTY(imesa);
    _swrast_CopyPixels(ctx, srcx, srcy, width, height, destx, desty, type);
}
static void
savageDrawPixels( GLcontext *ctx,
		  GLint x, GLint y,
		  GLsizei width, GLsizei height,
		  GLenum format, GLenum type,
		  const struct gl_pixelstore_attrib *packing,
		  const GLvoid *pixels )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    FLUSH_BATCH(imesa);
    WAIT_IDLE_EMPTY(imesa);
    _swrast_DrawPixels(ctx, x, y, width, height, format, type, packing, pixels);
}
static void
savageReadPixels( GLcontext *ctx,
		  GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type,
		  const struct gl_pixelstore_attrib *packing,
		  GLvoid *pixels )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    FLUSH_BATCH(imesa);
    WAIT_IDLE_EMPTY(imesa);
    _swrast_ReadPixels(ctx, x, y, width, height, format, type, packing, pixels);
}

/*
 * Make sure the hardware is idle when span-rendering.
 */
static void savageSpanRenderStart( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   FLUSH_BATCH(imesa);
   WAIT_IDLE_EMPTY(imesa);
}


void savageDDInitSpanFuncs( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart = savageSpanRenderStart;

   /* XXX these should probably be plugged in elsewhere */
   ctx->Driver.CopyPixels = savageCopyPixels;
   ctx->Driver.DrawPixels = savageDrawPixels;
   ctx->Driver.ReadPixels = savageReadPixels;
}



/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
savageSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis,
                       GLboolean float_depth)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         savageInitPointers_565(&drb->Base);
      }
      else {
         savageInitPointers_8888(&drb->Base);
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      if (float_depth) {
         savageInitDepthPointers_z16f(&drb->Base);
      }
      else {
         savageInitDepthPointers_z16(&drb->Base);
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      if (float_depth) {
         savageInitDepthPointers_s8_z24f(&drb->Base);
      }
      else {
         savageInitDepthPointers_s8_z24(&drb->Base);
      }
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      savageInitStencilPointers_s8_z24(&drb->Base);
   }
}
