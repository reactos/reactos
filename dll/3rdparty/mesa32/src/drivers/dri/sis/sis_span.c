/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
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
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_span.c,v 1.5 2001/03/21 16:14:26 dawes Exp $ */

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "sis_context.h"
#include "sis_span.h"
#include "sis_lock.h"
#include "sis_tris.h"

#include "swrast/swrast.h"

#define DBG 0

#define LOCAL_VARS							\
   sisContextPtr smesa = SIS_CONTEXT(ctx);				\
   __DRIdrawablePrivate *dPriv = smesa->driDrawable;			\
   struct sis_renderbuffer *srb = (struct sis_renderbuffer *) rb;	\
   GLuint pitch = srb->pitch;						\
   char *buf = srb->map;						\
   GLuint p;								\
   (void) buf; (void) p;
   

#define LOCAL_DEPTH_VARS						\
   sisContextPtr smesa = SIS_CONTEXT(ctx);				\
   __DRIdrawablePrivate *dPriv = smesa->driDrawable;			\
   struct sis_renderbuffer *srb = (struct sis_renderbuffer *) rb;	\
   char *buf = srb->map;

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS 

#define HW_LOCK() do {} while(0);

#define HW_UNLOCK() do {} while(0);

/* RGB565 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    sis##x##_RGB565
#define TAG2(x,y) sis##x##_RGB565##y
#include "spantmp2.h"


/* ARGB8888 */
/* FIXME the old code always read back alpha as 0xff, i.e. fully opaque.
   Was there a reason to do so ? If so that'll won't work with that template... */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    sis##x##_ARGB8888
#define TAG2(x,y) sis##x##_ARGB8888##y
#include "spantmp2.h"


/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLushort *)(buf + (_x)*2 + (_y)*srb->pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLushort *)(buf + (_x)*2 + (_y)*srb->pitch);

#define TAG(x) sis##x##_z16
#include "depthtmp.h"


/* 32 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLuint *)(buf + (_x)*4 + (_y)*srb->pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*srb->pitch);

#define TAG(x) sis##x##_z32
#include "depthtmp.h"


/* 8/24 bit interleaved depth/stencil functions
 */
#define WRITE_DEPTH( _x, _y, d ) {				\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*srb->pitch);	\
   tmp &= 0xff000000;						\
   tmp |= (d & 0x00ffffff);					\
   *(GLuint *)(buf + (_x)*4 + (_y)*srb->pitch) = tmp;		\
}

#define READ_DEPTH( d, _x, _y )	{				\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*srb->pitch) & 0x00ffffff; \
}

#define TAG(x) sis##x##_z24_s8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) {				\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*smesa->depth.pitch); \
   tmp &= 0x00ffffff;						\
   tmp |= (d << 24);						\
   *(GLuint *)(buf + (_x)*4 + (_y)*srb->pitch) = tmp;	\
}

#define READ_STENCIL( d, _x, _y )			\
   d = (*(GLuint *)(buf + (_x)*4 + (_y)*srb->pitch) & 0xff000000) >> 24;

#define TAG(x) sis##x##_z24_s8
#include "stenciltmp.h"



void sisSpanRenderStart( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   SIS_FIREVERTICES(smesa);
   LOCK_HARDWARE();
   WaitEngIdle( smesa );
}

void sisSpanRenderFinish( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   _swrast_flush( ctx );
   UNLOCK_HARDWARE();
}

void
sisDDInitSpanFuncs( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart   = sisSpanRenderStart;
   swdd->SpanRenderFinish  = sisSpanRenderFinish; 
}



/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
sisSetSpanFunctions(struct sis_renderbuffer *srb, const GLvisual *vis)
{
   if (srb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         sisInitPointers_RGB565( &srb->Base );
      }
      else {
         sisInitPointers_ARGB8888( &srb->Base );
      }
   }
   else if (srb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      sisInitDepthPointers_z16(&srb->Base);
   }
   else if (srb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      sisInitDepthPointers_z24_s8(&srb->Base);
   }
   else if (srb->Base.InternalFormat == GL_DEPTH_COMPONENT32) {
      sisInitDepthPointers_z32(&srb->Base);
   }
   else if (srb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      sisInitStencilPointers_z24_s8(&srb->Base);
   }
}
