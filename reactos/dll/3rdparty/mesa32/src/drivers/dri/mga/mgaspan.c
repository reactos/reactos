/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "mtypes.h"
#include "mgadd.h"
#include "mgacontext.h"
#include "mgaspan.h"
#include "mgaioctl.h"
#include "swrast/swrast.h"

#define DBG 0

#define LOCAL_VARS					\
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);		\
   __DRIscreenPrivate *sPriv = mmesa->driScreen;	\
   driRenderbuffer *drb = (driRenderbuffer *) rb;	\
   const __DRIdrawablePrivate *dPriv = drb->dPriv;	\
   GLuint pitch = drb->pitch;				\
   GLuint height = dPriv->h;				\
   char *buf = (char *)(sPriv->pFB +			\
			drb->offset +			\
			dPriv->x * drb->cpp +		\
			dPriv->y * pitch);		\
   GLuint p;						\
   (void) buf; (void) p



#define LOCAL_DEPTH_VARS						\
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);				\
   __DRIscreenPrivate *sPriv = mmesa->driScreen;			\
   driRenderbuffer *drb = (driRenderbuffer *) rb;			\
   const __DRIdrawablePrivate *dPriv = drb->dPriv;			\
   GLuint pitch = drb->pitch;						\
   GLuint height = dPriv->h;						\
   char *buf = (char *)(sPriv->pFB +					\
			drb->offset +					\
			dPriv->x * drb->cpp +				\
			dPriv->y * pitch)

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS 

#define HW_LOCK()

/* FIXME could/should we use dPriv->numClipRects like the other drivers? */
#define HW_CLIPLOOP()						\
  do {								\
    int _nc = mmesa->numClipRects;				\
    while (_nc--) {						\
       int minx = mmesa->pClipRects[_nc].x1 - mmesa->drawX;	\
       int miny = mmesa->pClipRects[_nc].y1 - mmesa->drawY;	\
       int maxx = mmesa->pClipRects[_nc].x2 - mmesa->drawX;	\
       int maxy = mmesa->pClipRects[_nc].y2 - mmesa->drawY;

#define HW_ENDCLIPLOOP()			\
    }						\
  } while (0)

#define HW_UNLOCK()



#define Y_FLIP(_y) (height - _y - 1)

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    mga##x##_565
#define TAG2(x,y) mga##x##_565##y
#include "spantmp2.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    mga##x##_8888
#define TAG2(x,y) mga##x##_8888##y
#include "spantmp2.h"


/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLushort *)(buf + (_x)*2 + (_y)*pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLushort *)(buf + (_x)*2 + (_y)*pitch);

#define TAG(x) mga##x##_z16
#include "depthtmp.h"




/* 32 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLuint *)(buf + (_x)*4 + (_y)*pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*pitch);

#define TAG(x) mga##x##_z32
#include "depthtmp.h"



/* 24/8 bit interleaved depth/stencil functions
 */
#define WRITE_DEPTH( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*pitch);	\
   tmp &= 0xff;						\
   tmp |= (d) << 8;					\
   *(GLuint *)(buf + (_x)*4 + (_y)*pitch) = tmp;		\
}

#define READ_DEPTH( d, _x, _y )	{				\
   d = (*(GLuint *)(buf + (_x)*4 + (_y)*pitch) & ~0xff) >> 8;	\
}

#define TAG(x) mga##x##_z24_s8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   tmp &= 0xffffff00;					\
   tmp |= d & 0xff;					\
   *(GLuint *)(buf + _x*4 + _y*pitch) = tmp;		\
}

#define READ_STENCIL( d, _x, _y )		\
   d = *(GLuint *)(buf + _x*4 + _y*pitch) & 0xff;

#define TAG(x) mga##x##_z24_s8
#include "stenciltmp.h"


static void
mgaSpanRenderStart( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   FLUSH_BATCH( mmesa );
   LOCK_HARDWARE_QUIESCENT( mmesa );
}

static void
mgaSpanRenderFinish( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   _swrast_flush( ctx );
   UNLOCK_HARDWARE( mmesa );
}

/**
 * Initialize the driver callbacks for the read / write span functions.
 *
 * \bug
 * To really support RGB888 and RGBA8888 visuals, we need separate read and
 * write routines for 888 and 8888.  We also need to determine whether or not
 * the visual has destination alpha.
 */
void mgaDDInitSpanFuncs( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart = mgaSpanRenderStart;
   swdd->SpanRenderFinish = mgaSpanRenderFinish;
}


/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
mgaSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         mgaInitPointers_565(&drb->Base);
      }
      else {
         mgaInitPointers_8888(&drb->Base);
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      mgaInitDepthPointers_z16(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      mgaInitDepthPointers_z24_s8(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT32) {
      mgaInitDepthPointers_z32(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      mgaInitStencilPointers_z24_s8(&drb->Base);
   }
}
