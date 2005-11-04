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
    struct via_context *vmesa = VIA_CONTEXT(ctx);             		\
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;                	\
    GLuint draw_pitch = vmesa->drawBuffer->pitch;                       \
    GLuint read_pitch = vmesa->readBuffer->pitch;                       \
    GLuint height = dPriv->h;                                        	\
    GLint p = 0;							\
    char *buf = (char *)(vmesa->drawBuffer->origMap + vmesa->drawXoff * vmesa->viaScreen->bytesPerPixel); \
    char *read_buf = (char *)(vmesa->readBuffer->origMap + vmesa->drawXoff * vmesa->viaScreen->bytesPerPixel); \
    (void) (read_pitch && draw_pitch && buf && read_buf && p);

/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define GET_SRC_PTR(_x, _y) (read_buf + (_x) * 2 + (_y) * read_pitch)
#define GET_DST_PTR(_x, _y) (     buf + (_x) * 2 + (_y) * draw_pitch)
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    via##x##_565
#define TAG2(x,y) via##x##_565##y
#include "spantmp2.h"


/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define GET_SRC_PTR(_x, _y) (read_buf + (_x) * 4 + (_y) * read_pitch)
#define GET_DST_PTR(_x, _y) (     buf + (_x) * 4 + (_y) * draw_pitch)
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    via##x##_8888
#define TAG2(x,y) via##x##_8888##y
#include "spantmp2.h"


/* 16 bit depthbuffer functions.
 */
#define LOCAL_DEPTH_VARS                                \
    struct via_context *vmesa = VIA_CONTEXT(ctx);             \
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;   \
    GLuint depth_pitch = vmesa->depth.pitch;                  \
    GLuint height = dPriv->h;                           \
    char *buf = (char *)(vmesa->depth.map + (vmesa->drawXoff * vmesa->depth.bpp/8))

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS 


#define WRITE_DEPTH(_x, _y, d)                      \
    *(GLushort *)(buf + (_x) * 2 + (_y) * depth_pitch) = d;

#define READ_DEPTH(d, _x, _y)                       \
    d = *(volatile GLushort *)(buf + (_x) * 2 + (_y) * depth_pitch);

#define TAG(x) via##x##_16
#include "depthtmp.h"

/* 32 bit depthbuffer functions.
 */
#define WRITE_DEPTH(_x, _y, d)                      \
    *(GLuint *)(buf + (_x) * 4 + (_y) * depth_pitch) = d;

#define READ_DEPTH(d, _x, _y)                       \
    d = *(volatile GLuint *)(buf + (_x) * 4 + (_y) * depth_pitch);

#define TAG(x) via##x##_32
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


#define TAG(x) via##x##_24_8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*depth_pitch);	\
   tmp &= 0xffffff00;					\
   tmp |= (d);					\
   *(GLuint *)(buf + (_x)*4 + (_y)*depth_pitch) = tmp;		\
}

#define READ_STENCIL( d, _x, _y )			\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*depth_pitch) & 0xff;

#define TAG(x) via##x##_24_8
#include "stenciltmp.h"



static void viaSetBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                      GLuint bufferBit)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);

    if (bufferBit == BUFFER_BIT_FRONT_LEFT) {
	vmesa->drawBuffer = vmesa->readBuffer = &vmesa->front;
    }
    else if (bufferBit == BUFFER_BIT_BACK_LEFT) {
	vmesa->drawBuffer = vmesa->readBuffer = &vmesa->back;
    }
    else {
        ASSERT(0);
    }
}

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
#if 0
    struct via_context *vmesa = VIA_CONTEXT(ctx);
#endif
    struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

    swdd->SetBuffer = viaSetBuffer;
#if 0
    if (vmesa->viaScreen->bitsPerPixel == 16) {
	viaInitPointers_565( swdd );
    }
    else if (vmesa->viaScreen->bitsPerPixel == 32) {
	viaInitPointers_8888( swdd );
    }
    else {
	assert(0);
    }
#endif
#if 0
    if (vmesa->glCtx->Visual.depthBits == 16) {
	swdd->ReadDepthSpan = viaReadDepthSpan_16;
	swdd->WriteDepthSpan = viaWriteDepthSpan_16;
	swdd->WriteMonoDepthSpan = viaWriteMonoDepthSpan_16;
	swdd->ReadDepthPixels = viaReadDepthPixels_16;
	swdd->WriteDepthPixels = viaWriteDepthPixels_16;
    }
    else if (vmesa->glCtx->Visual.depthBits == 24) {
        swdd->ReadDepthSpan = viaReadDepthSpan_24_8;
	swdd->WriteDepthSpan = viaWriteDepthSpan_24_8;
	swdd->WriteMonoDepthSpan = viaWriteMonoDepthSpan_24_8;
	swdd->ReadDepthPixels = viaReadDepthPixels_24_8;
	swdd->WriteDepthPixels = viaWriteDepthPixels_24_8;

	swdd->WriteStencilSpan = viaWriteStencilSpan_24_8;
	swdd->ReadStencilSpan = viaReadStencilSpan_24_8;
	swdd->WriteStencilPixels = viaWriteStencilPixels_24_8;
	swdd->ReadStencilPixels = viaReadStencilPixels_24_8;
    }
    else if (vmesa->glCtx->Visual.depthBits == 32) {
	swdd->ReadDepthSpan = viaReadDepthSpan_32;
	swdd->WriteDepthSpan = viaWriteDepthSpan_32;
	swdd->WriteMonoDepthSpan = viaWriteMonoDepthSpan_32;
	swdd->ReadDepthPixels = viaReadDepthPixels_32;
	swdd->WriteDepthPixels = viaWriteDepthPixels_32;
    }
#endif

    swdd->SpanRenderStart = viaSpanRenderStart;
    swdd->SpanRenderFinish = viaSpanRenderFinish; 

#if 0
    swdd->WriteCI8Span = NULL;
    swdd->WriteCI32Span = NULL;
    swdd->WriteMonoCISpan = NULL;
    swdd->WriteCI32Pixels = NULL;
    swdd->WriteMonoCIPixels = NULL;
    swdd->ReadCI32Span = NULL;
    swdd->ReadCI32Pixels = NULL;	
#endif
}



/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
viaSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         viaInitPointers_565(&drb->Base);
      }
      else {
         viaInitPointers_8888(&drb->Base);
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      drb->Base.GetRow        = viaReadDepthSpan_16;
      drb->Base.GetValues     = viaReadDepthPixels_16;
      drb->Base.PutRow        = viaWriteDepthSpan_16;
      drb->Base.PutMonoRow    = viaWriteMonoDepthSpan_16;
      drb->Base.PutValues     = viaWriteDepthPixels_16;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      drb->Base.GetRow        = viaReadDepthSpan_24_8;
      drb->Base.GetValues     = viaReadDepthPixels_24_8;
      drb->Base.PutRow        = viaWriteDepthSpan_24_8;
      drb->Base.PutMonoRow    = viaWriteMonoDepthSpan_24_8;
      drb->Base.PutValues     = viaWriteDepthPixels_24_8;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT32) {
      drb->Base.GetRow        = viaReadDepthSpan_32;
      drb->Base.GetValues     = viaReadDepthPixels_32;
      drb->Base.PutRow        = viaWriteDepthSpan_32;
      drb->Base.PutMonoRow    = viaWriteMonoDepthSpan_32;
      drb->Base.PutValues     = viaWriteDepthPixels_32;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      drb->Base.GetRow        = viaReadStencilSpan_24_8;
      drb->Base.GetValues     = viaReadStencilPixels_24_8;
      drb->Base.PutRow        = viaWriteStencilSpan_24_8;
      drb->Base.PutMonoRow    = viaWriteMonoStencilSpan_24_8;
      drb->Base.PutValues     = viaWriteStencilPixels_24_8;
      drb->Base.PutMonoValues = NULL;
   }
}
