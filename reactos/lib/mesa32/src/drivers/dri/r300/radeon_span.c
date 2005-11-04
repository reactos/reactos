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
#include "radeon_ioctl.h"
#include "r300_ioctl.h"
#include "radeon_span.h"

#define DBG 0

#define LOCAL_VARS							\
   radeonContextPtr radeon = RADEON_CONTEXT(ctx);			\
   driRenderbuffer* drb = (driRenderbuffer*)rb;				\
   __DRIscreenPrivate *sPriv = radeon->dri.screen;			\
   __DRIdrawablePrivate *dPriv = radeon->dri.drawable;			\
   GLuint pitch = drb->pitch * drb->cpp;				\
   GLuint height = dPriv->h;						\
   char *buf = (char *)(sPriv->pFB +					\
			drb->offset +					\
			(dPriv->x * drb->cpp) +				\
			(dPriv->y * pitch));				\
   GLuint p;								\
   (void) p

#define LOCAL_DEPTH_VARS						\
   radeonContextPtr radeon = RADEON_CONTEXT(ctx);			\
   driRenderbuffer* drb = (driRenderbuffer*)rb;				\
   __DRIscreenPrivate *sPriv = radeon->dri.screen;			\
   __DRIdrawablePrivate *dPriv = radeon->dri.drawable;			\
   GLuint pitch = drb->pitch;						\
   GLuint height = dPriv->h;						\
   GLuint xo = dPriv->x;						\
   GLuint yo = dPriv->y;						\
   char *buf = (char *)(sPriv->pFB + drb->offset);			\
   (void) buf; (void) pitch

#define LOCAL_STENCIL_VARS	LOCAL_DEPTH_VARS

#define CLIPPIXEL( _x, _y )						\
   ((_x >= minx) && (_x < maxx) && (_y >= miny) && (_y < maxy))

#define CLIPSPAN( _x, _y, _n, _x1, _n1, _i )				\
   if ( _y < miny || _y >= maxy ) {					\
      _n1 = 0, _x1 = x;							\
   } else {								\
      _n1 = _n;								\
      _x1 = _x;								\
      if ( _x1 < minx ) _i += (minx-_x1), _n1 -= (minx-_x1), _x1 = minx; \
      if ( _x1 + _n1 >= maxx ) n1 -= (_x1 + _n1 - maxx);		\
   }

#define Y_FLIP( _y )		(height - _y - 1)

#define HW_LOCK()

#define HW_CLIPLOOP()							\
   do {									\
      int _nc = dPriv->numClipRects;					\
									\
      while ( _nc-- ) {							\
	 int minx = dPriv->pClipRects[_nc].x1 - dPriv->x;		\
	 int miny = dPriv->pClipRects[_nc].y1 - dPriv->y;		\
	 int maxx = dPriv->pClipRects[_nc].x2 - dPriv->x;		\
	 int maxy = dPriv->pClipRects[_nc].y2 - dPriv->y;

#define HW_ENDCLIPLOOP()						\
      }									\
   } while (0)

#define HW_UNLOCK()

/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_565( color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch) = ((((int)r & 0xf8) << 8) |	\
					   (((int)g & 0xfc) << 3) |	\
					   (((int)b & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )					\
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
   do {									\
      GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);		\
      rgba[0] = ((p >> 8) & 0xf8) * 255 / 0xf8;				\
      rgba[1] = ((p >> 3) & 0xfc) * 255 / 0xfc;				\
      rgba[2] = ((p << 3) & 0xf8) * 255 / 0xf8;				\
      rgba[3] = 0xff;							\
   } while (0)

#define TAG(x) radeon##x##_RGB565
#include "spantmp.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_8888( color[3], color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a )			\
do {								\
   *(GLuint *)(buf + _x*4 + _y*pitch) = ((b <<  0) |		\
					 (g <<  8) |		\
					 (r << 16) |		\
					 (a << 24) );		\
} while (0)

#define WRITE_PIXEL( _x, _y, p ) 			\
do {							\
   *(GLuint *)(buf + _x*4 + _y*pitch) = p;		\
} while (0)

#define READ_RGBA( rgba, _x, _y )				\
do {								\
   volatile GLuint *ptr = (volatile GLuint *)(buf + _x*4 + _y*pitch); \
   GLuint p = *ptr;					\
   rgba[0] = (p >> 16) & 0xff;					\
   rgba[1] = (p >>  8) & 0xff;					\
   rgba[2] = (p >>  0) & 0xff;					\
   rgba[3] = (p >> 24) & 0xff;					\
} while (0)

#define TAG(x) radeon##x##_ARGB8888
#include "spantmp.h"

/* ================================================================
 * Depth buffer
 */

/* 16-bit depth buffer functions
 */
#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)(buf + (_x + xo + (_y + yo)*pitch)*2 ) = d;

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)(buf + (_x + xo + (_y + yo)*pitch)*2 );

#define TAG(x) radeon##x##_16_LINEAR
#include "depthtmp.h"

/* 24 bit depth, 8 bit stencil depthbuffer functions
 *
 * Careful: It looks like the R300 uses ZZZS byte order while the R200
 * uses SZZZ for 24 bit depth, 8 bit stencil mode.
 */
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint offset = ((_x) + xo + ((_y) + yo)*pitch)*4;			\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0x000000ff;							\
   tmp |= ((d << 8) & 0xffffff00);					\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)

#define READ_DEPTH( d, _x, _y )						\
do { \
   d = (*(GLuint *)(buf + ((_x) + xo + ((_y) + yo)*pitch)*4) & 0xffffff00) >> 8; \
} while(0)

#define TAG(x) radeon##x##_24_8_LINEAR
#include "depthtmp.h"

/* ================================================================
 * Stencil buffer
 */

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint offset = (_x + xo + (_y + yo)*pitch)*4;			\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xffffff00;							\
   tmp |= (d) & 0xff;							\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)

#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint offset = (_x + xo + (_y + yo)*pitch)*4;			\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   d = tmp & 0x000000ff;						\
} while (0)

#define TAG(x) radeon##x##_24_8_LINEAR
#include "stenciltmp.h"

/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void radeonSetBuffer(GLcontext * ctx,
			  GLframebuffer * colorBuffer, GLuint bufferBit)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	int buffer;

	switch (bufferBit) {
	case BUFFER_BIT_FRONT_LEFT:
		buffer = 0;
		break;

	case BUFFER_BIT_BACK_LEFT:
		buffer = 1;
		break;

	default:
		_mesa_problem(ctx, "Bad bufferBit in %s", __FUNCTION__);
		return;
	}

	if (radeon->doPageFlip && radeon->sarea->pfCurrentPage == 1)
		buffer ^= 1;

#if 0
	fprintf(stderr, "%s: using %s buffer\n", __FUNCTION__,
		buffer ? "back" : "front");
#endif

	if (buffer) {
		radeon->state.pixel.readOffset =
			radeon->radeonScreen->backOffset;
		radeon->state.pixel.readPitch =
			radeon->radeonScreen->backPitch;
		radeon->state.color.drawOffset =
			radeon->radeonScreen->backOffset;
		radeon->state.color.drawPitch =
			radeon->radeonScreen->backPitch;
	} else {
		radeon->state.pixel.readOffset =
			radeon->radeonScreen->frontOffset;
		radeon->state.pixel.readPitch =
			radeon->radeonScreen->frontPitch;
		radeon->state.color.drawOffset =
			radeon->radeonScreen->frontOffset;
		radeon->state.color.drawPitch =
			radeon->radeonScreen->frontPitch;
	}
}

/* Move locking out to get reasonable span performance (10x better
 * than doing this in HW_LOCK above).  WaitForIdle() is the main
 * culprit.
 */

static void radeonSpanRenderStart(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	if (IS_FAMILY_R200(radeon))
		R200_FIREVERTICES((r200ContextPtr)radeon);
	else
		r300Flush(ctx);

	LOCK_HARDWARE(radeon);
	radeonWaitForIdleLocked(radeon);

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
		volatile int *read_buf =
		    (volatile int *)(radeon->dri.screen->pFB +
				     radeon->state.pixel.readOffset);
		p = *read_buf;
		*read_buf = p;
	}
}

static void radeonSpanRenderFinish(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);

	_swrast_flush(ctx);
	UNLOCK_HARDWARE(radeon);
}

void radeonInitSpanFuncs(GLcontext * ctx)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct swrast_device_driver *swdd =
	    _swrast_GetDeviceDriverReference(ctx);

	swdd->SetBuffer = radeonSetBuffer;

	swdd->SpanRenderStart = radeonSpanRenderStart;
	swdd->SpanRenderFinish = radeonSpanRenderFinish;
}

/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void radeonSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
	if (drb->Base.InternalFormat == GL_RGBA) {
		if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
			drb->Base.GetRow        = radeonReadRGBASpan_RGB565;
			drb->Base.GetValues     = radeonReadRGBAPixels_RGB565;
			drb->Base.PutRow        = radeonWriteRGBASpan_RGB565;
			drb->Base.PutRowRGB     = radeonWriteRGBSpan_RGB565;
			drb->Base.PutMonoRow    = radeonWriteMonoRGBASpan_RGB565;
			drb->Base.PutValues     = radeonWriteRGBAPixels_RGB565;
			drb->Base.PutMonoValues = radeonWriteMonoRGBAPixels_RGB565;
		}
		else {
			drb->Base.GetRow        = radeonReadRGBASpan_ARGB8888;
			drb->Base.GetValues     = radeonReadRGBAPixels_ARGB8888;
			drb->Base.PutRow        = radeonWriteRGBASpan_ARGB8888;
			drb->Base.PutRowRGB     = radeonWriteRGBSpan_ARGB8888;
			drb->Base.PutMonoRow    = radeonWriteMonoRGBASpan_ARGB8888;
			drb->Base.PutValues     = radeonWriteRGBAPixels_ARGB8888;
			drb->Base.PutMonoValues = radeonWriteMonoRGBAPixels_ARGB8888;
		}
	}
	else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
		drb->Base.GetRow        = radeonReadDepthSpan_16_LINEAR;
		drb->Base.GetValues     = radeonReadDepthPixels_16_LINEAR;
		drb->Base.PutRow        = radeonWriteDepthSpan_16_LINEAR;
		drb->Base.PutMonoRow    = radeonWriteMonoDepthSpan_16_LINEAR;
		drb->Base.PutValues     = radeonWriteDepthPixels_16_LINEAR;
		drb->Base.PutMonoValues = NULL;
	}
	else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
		drb->Base.GetRow        = radeonReadDepthSpan_24_8_LINEAR;
		drb->Base.GetValues     = radeonReadDepthPixels_24_8_LINEAR;
		drb->Base.PutRow        = radeonWriteDepthSpan_24_8_LINEAR;
		drb->Base.PutMonoRow    = radeonWriteMonoDepthSpan_24_8_LINEAR;
		drb->Base.PutValues     = radeonWriteDepthPixels_24_8_LINEAR;
		drb->Base.PutMonoValues = NULL;
	}
	else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
		drb->Base.GetRow        = radeonReadStencilSpan_24_8_LINEAR;
		drb->Base.GetValues     = radeonReadStencilPixels_24_8_LINEAR;
		drb->Base.PutRow        = radeonWriteStencilSpan_24_8_LINEAR;
		drb->Base.PutMonoRow    = radeonWriteMonoStencilSpan_24_8_LINEAR;
		drb->Base.PutValues     = radeonWriteStencilPixels_24_8_LINEAR;
		drb->Base.PutMonoValues = NULL;
	}
}

