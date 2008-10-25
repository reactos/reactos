/**************************************************************************

Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.
Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

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
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#include "glheader.h"
#include "swrast/swrast.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_span.h"
#include "radeon_tex.h"

#include "drirenderbuffer.h"

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

#define TAG(x)    radeon##x##_RGB565
#define TAG2(x,y) radeon##x##_RGB565##y
#define GET_PTR(X,Y) (buf + ((Y) * drb->flippedPitch + (X)) * 2)
#include "spantmp2.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    radeon##x##_ARGB8888
#define TAG2(x,y) radeon##x##_ARGB8888##y
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
 * are set up correctly. It is not quite enough to get it working with hyperz
 * too...
 */

static GLuint radeon_mba_z32(const driRenderbuffer * drb, GLint x, GLint y)
{
	GLuint pitch = drb->pitch;
	if (drb->depthHasSurface) {
		return 4 * (x + y * pitch);
	} else {
		GLuint ba, address = 0;	/* a[0..1] = 0           */

#ifdef COMPILE_R300
		ba = (y / 8) * (pitch / 8) + (x / 8);
#else
		ba = (y / 16) * (pitch / 16) + (x / 16);
#endif

		address |= (x & 0x7) << 2;	/* a[2..4] = x[0..2]     */
		address |= (y & 0x3) << 5;	/* a[5..6] = y[0..1]     */
		address |= (((x & 0x10) >> 2) ^ (y & 0x4)) << 5;	/* a[7]    = x[4] ^ y[2] */
		address |= (ba & 0x3) << 8;	/* a[8..9] = ba[0..1]    */

		address |= (y & 0x8) << 7;	/* a[10]   = y[3]        */
		address |= (((x & 0x8) << 1) ^ (y & 0x10)) << 7;	/* a[11]   = x[3] ^ y[4] */
		address |= (ba & ~0x3) << 10;	/* a[12..] = ba[2..]     */

		return address;
	}
}

static INLINE GLuint
radeon_mba_z16(const driRenderbuffer * drb, GLint x, GLint y)
{
	GLuint pitch = drb->pitch;
	if (drb->depthHasSurface) {
		return 2 * (x + y * pitch);
	} else {
		GLuint ba, address = 0;	/* a[0]    = 0           */

		ba = (y / 16) * (pitch / 32) + (x / 32);

		address |= (x & 0x7) << 1;	/* a[1..3] = x[0..2]     */
		address |= (y & 0x7) << 4;	/* a[4..6] = y[0..2]     */
		address |= (x & 0x8) << 4;	/* a[7]    = x[3]        */
		address |= (ba & 0x3) << 8;	/* a[8..9] = ba[0..1]    */
		address |= (y & 0x8) << 7;	/* a[10]   = y[3]        */
		address |= ((x & 0x10) ^ (y & 0x10)) << 7;	/* a[11]   = x[4] ^ y[4] */
		address |= (ba & ~0x3) << 10;	/* a[12..] = ba[2..]     */

		return address;
	}
}

/* 16-bit depth buffer functions
 */
#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)(buf + radeon_mba_z16( drb, _x + xo, _y + yo )) = d;

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)(buf + radeon_mba_z16( drb, _x + xo, _y + yo ));

#define TAG(x) radeon##x##_z16
#include "depthtmp.h"

/* 24 bit depth, 8 bit stencil depthbuffer functions
 *
 * Careful: It looks like the R300 uses ZZZS byte order while the R200
 * uses SZZZ for 24 bit depth, 8 bit stencil mode.
 */
#ifdef COMPILE_R300
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint offset = radeon_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0x000000ff;							\
   tmp |= ((d << 8) & 0xffffff00);					\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)
#else
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint offset = radeon_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xff000000;							\
   tmp |= ((d) & 0x00ffffff);						\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)
#endif

#ifdef COMPILE_R300
#define READ_DEPTH( d, _x, _y )						\
  do { \
    d = (*(GLuint *)(buf + radeon_mba_z32( drb, _x + xo,		\
					 _y + yo )) & 0xffffff00) >> 8; \
  }while(0)
#else
#define READ_DEPTH( d, _x, _y )						\
   d = *(GLuint *)(buf + radeon_mba_z32( drb, _x + xo,			\
					 _y + yo )) & 0x00ffffff;
#endif

#define TAG(x) radeon##x##_z24_s8
#include "depthtmp.h"

/* ================================================================
 * Stencil buffer
 */

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#ifdef COMPILE_R300
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint offset = radeon_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xffffff00;							\
   tmp |= (d) & 0xff;							\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)
#else
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint offset = radeon_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0x00ffffff;							\
   tmp |= (((d) & 0xff) << 24);						\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)
#endif

#ifdef COMPILE_R300
#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint offset = radeon_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   d = tmp & 0x000000ff;						\
} while (0)
#else
#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint offset = radeon_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   d = (tmp & 0xff000000) >> 24;					\
} while (0)
#endif

#define TAG(x) radeon##x##_z24_s8
#include "stenciltmp.h"

/* Move locking out to get reasonable span performance (10x better
 * than doing this in HW_LOCK above).  WaitForIdle() is the main
 * culprit.
 */

static void radeonSpanRenderStart(GLcontext * ctx)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
#ifdef COMPILE_R300
	r300ContextPtr r300 = (r300ContextPtr) rmesa;
	R300_FIREVERTICES(r300);
#else
	RADEON_FIREVERTICES(rmesa);
#endif
	LOCK_HARDWARE(rmesa);
	radeonWaitForIdleLocked(rmesa);
}

static void radeonSpanRenderFinish(GLcontext * ctx)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	_swrast_flush(ctx);
	UNLOCK_HARDWARE(rmesa);
}

void radeonInitSpanFuncs(GLcontext * ctx)
{
	struct swrast_device_driver *swdd =
	    _swrast_GetDeviceDriverReference(ctx);
	swdd->SpanRenderStart = radeonSpanRenderStart;
	swdd->SpanRenderFinish = radeonSpanRenderFinish;
}

/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void radeonSetSpanFunctions(driRenderbuffer * drb, const GLvisual * vis)
{
	if (drb->Base.InternalFormat == GL_RGBA) {
		if (vis->redBits == 5 && vis->greenBits == 6
		    && vis->blueBits == 5) {
			radeonInitPointers_RGB565(&drb->Base);
		} else {
			radeonInitPointers_ARGB8888(&drb->Base);
		}
	} else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
		radeonInitDepthPointers_z16(&drb->Base);
	} else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
		radeonInitDepthPointers_z24_s8(&drb->Base);
	} else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
		radeonInitStencilPointers_z24_s8(&drb->Base);
	}
}
