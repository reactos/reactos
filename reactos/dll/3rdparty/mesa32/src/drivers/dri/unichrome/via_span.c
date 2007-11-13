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

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "colormac.h"
#include "via_context.h"
#include "via_span.h"
#include "via_ioctl.h"
#include "swrast/swrast.h"

#define DBG 0

#define Y_FLIP(_y) (height - _y - 1)

#define HW_LOCK() 

#define HW_UNLOCK()

#undef LOCAL_VARS
#define LOCAL_VARS                                                   	\
    struct via_renderbuffer *vrb = (struct via_renderbuffer *) rb;   	\
    __DRIdrawablePrivate *dPriv = vrb->dPriv;                           \
    GLuint pitch = vrb->pitch;                                          \
    GLuint height = dPriv->h;                                        	\
    GLint p = 0;							\
    char *buf = (char *)(vrb->origMap);					\
    (void) p;

/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define GET_PTR(_x, _y) (buf + (_x) * 2 + (_y) * pitch)
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    via##x##_565
#define TAG2(x,y) via##x##_565##y
#include "spantmp2.h"


/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define GET_PTR(_x, _y) (buf + (_x) * 4 + (_y) * pitch)
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    via##x##_8888
#define TAG2(x,y) via##x##_8888##y
#include "spantmp2.h"


/* 16 bit depthbuffer functions.
 */
#define LOCAL_DEPTH_VARS                                            \
    struct via_renderbuffer *vrb = (struct via_renderbuffer *) rb;  \
    __DRIdrawablePrivate *dPriv = vrb->dPriv;                       \
    GLuint depth_pitch = vrb->pitch;                                \
    GLuint height = dPriv->h;                                       \
    char *buf = (char *)(vrb->map)

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS 


#define WRITE_DEPTH(_x, _y, d)                      \
    *(GLushort *)(buf + (_x) * 2 + (_y) * depth_pitch) = d;

#define READ_DEPTH(d, _x, _y)                       \
    d = *(volatile GLushort *)(buf + (_x) * 2 + (_y) * depth_pitch);

#define TAG(x) via##x##_z16
#include "depthtmp.h"

/* 32 bit depthbuffer functions.
 */
#define WRITE_DEPTH(_x, _y, d)                      \
    *(GLuint *)(buf + (_x) * 4 + (_y) * depth_pitch) = d;

#define READ_DEPTH(d, _x, _y)                       \
    d = *(volatile GLuint *)(buf + (_x) * 4 + (_y) * depth_pitch);

#define TAG(x) via##x##_z32
#include "depthtmp.h"



/* 24/8 bit interleaved depth/stencil functions
 */
#define WRITE_DEPTH( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*depth_pitch);	\
   tmp &= 0x000000ff;					\
   tmp |= ((d)<<8);				\
   *(GLuint *)(buf + (_x)*4 + (_y)*depth_pitch) = tmp;		\
}

#define READ_DEPTH( d, _x, _y )		\
   d = (*(GLuint *)(buf + (_x)*4 + (_y)*depth_pitch)) >> 8;


#define TAG(x) via##x##_z24_s8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*depth_pitch);	\
   tmp &= 0xffffff00;					\
   tmp |= (d);					\
   *(GLuint *)(buf + (_x)*4 + (_y)*depth_pitch) = tmp;		\
}

#define READ_STENCIL( d, _x, _y )			\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*depth_pitch) & 0xff;

#define TAG(x) via##x##_z24_s8
#include "stenciltmp.h"




/* Move locking out to get reasonable span performance.
 */
void viaSpanRenderStart( GLcontext *ctx )
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);     
   viaWaitIdle(vmesa, GL_FALSE);
   LOCK_HARDWARE(vmesa);
}

void viaSpanRenderFinish( GLcontext *ctx )
{
   struct via_context *vmesa = VIA_CONTEXT(ctx);
   _swrast_flush( ctx );
   UNLOCK_HARDWARE( vmesa );
}

void viaInitSpanFuncs(GLcontext *ctx)
{
    struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
    swdd->SpanRenderStart = viaSpanRenderStart;
    swdd->SpanRenderFinish = viaSpanRenderFinish; 
}



/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
viaSetSpanFunctions(struct via_renderbuffer *vrb, const GLvisual *vis)
{
   if (vrb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         viaInitPointers_565(&vrb->Base);
      }
      else {
         viaInitPointers_8888(&vrb->Base);
      }
   }
   else if (vrb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      viaInitDepthPointers_z16(&vrb->Base);
   }
   else if (vrb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      viaInitDepthPointers_z24_s8(&vrb->Base);
   }
   else if (vrb->Base.InternalFormat == GL_DEPTH_COMPONENT32) {
      viaInitDepthPointers_z32(&vrb->Base);
   }
   else if (vrb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      viaInitStencilPointers_z24_s8(&vrb->Base);
   }
}
