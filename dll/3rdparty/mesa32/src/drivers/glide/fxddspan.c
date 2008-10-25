/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Authors:
 *    David Bucciarelli
 *    Brian Paul
 *    Daryll Strauss
 *    Keith Whitwell
 *    Daniel Borca
 *    Hiroshi Morii
 */


/* fxdd.c - 3Dfx VooDoo Mesa span and pixel functions */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"
#include "fxglidew.h"
#include "swrast/swrast.h"


/************************************************************************/
/*****                    Span functions                            *****/
/************************************************************************/

#define DBG 0


#define LOCAL_VARS							\
    GrBuffer_t currentFB = GR_BUFFER_BACKBUFFER;			\
    GLuint pitch = info.strideInBytes;					\
    GLuint height = fxMesa->height;					\
    char *buf = (char *)((char *)info.lfbPtr + 0 /* x, y offset */);	\
    GLuint p;								\
    (void) buf; (void) p;

#define CLIPPIXEL( _x, _y )	( _x >= minx && _x < maxx &&		\
				  _y >= miny && _y < maxy )

#define CLIPSPAN( _x, _y, _n, _x1, _n1, _i )				\
    if ( _y < miny || _y >= maxy ) {					\
	_n1 = 0, _x1 = x;						\
    } else {								\
	_n1 = _n;							\
	_x1 = _x;							\
	if ( _x1 < minx ) _i += (minx-_x1), n1 -= (minx-_x1), _x1 = minx;\
	if ( _x1 + _n1 >= maxx ) n1 -= (_x1 + n1 - maxx);		\
    }

#define Y_FLIP(_y)		(height - _y - 1)

#define HW_WRITE_LOCK()							\
    fxMesaContext fxMesa = FX_CONTEXT(ctx);				\
    GrLfbInfo_t info;							\
    info.size = sizeof(GrLfbInfo_t);					\
    if ( grLfbLock( GR_LFB_WRITE_ONLY,					\
                   currentFB, LFB_MODE,					\
		   GR_ORIGIN_UPPER_LEFT, FXFALSE, &info ) ) {

#define HW_WRITE_UNLOCK()						\
	grLfbUnlock( GR_LFB_WRITE_ONLY, currentFB );			\
    }

#define HW_READ_LOCK()							\
    fxMesaContext fxMesa = FX_CONTEXT(ctx);				\
    GrLfbInfo_t info;							\
    info.size = sizeof(GrLfbInfo_t);					\
    if ( grLfbLock( GR_LFB_READ_ONLY, currentFB,			\
                    LFB_MODE, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info ) ) {

#define HW_READ_UNLOCK()						\
	grLfbUnlock( GR_LFB_READ_ONLY, currentFB );			\
    }

#define HW_WRITE_CLIPLOOP()						\
    do {								\
	/* remember, we need to flip the scissor, too */		\
	/* is it better to do it inside fxDDScissor? */			\
	const int minx = fxMesa->clipMinX;				\
	const int maxy = Y_FLIP(fxMesa->clipMinY);			\
	const int maxx = fxMesa->clipMaxX;				\
	const int miny = Y_FLIP(fxMesa->clipMaxY);

#define HW_READ_CLIPLOOP()						\
    do {								\
	/* remember, we need to flip the scissor, too */		\
	/* is it better to do it inside fxDDScissor? */			\
	const int minx = fxMesa->clipMinX;				\
	const int maxy = Y_FLIP(fxMesa->clipMinY);			\
	const int maxx = fxMesa->clipMaxX;				\
	const int miny = Y_FLIP(fxMesa->clipMaxY);

#define HW_ENDCLIPLOOP()						\
    } while (0)


/* 16 bit, ARGB1555 color spanline and pixel functions */

#undef LFB_MODE
#define LFB_MODE	GR_LFBWRITEMODE_1555

#undef BYTESPERPIXEL
#define BYTESPERPIXEL 2

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
    p = TDFXPACKCOLOR1555( color[RCOMP], color[GCOMP], color[BCOMP], color[ACOMP] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
    *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch) =			\
					TDFXPACKCOLOR1555( r, g, b, a )

#define WRITE_PIXEL( _x, _y, p )					\
    *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
    do {								\
	GLushort p = *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch);	\
	rgba[0] = FX_rgb_scale_5[(p >> 10) & 0x1F];			\
	rgba[1] = FX_rgb_scale_5[(p >> 5)  & 0x1F];			\
	rgba[2] = FX_rgb_scale_5[ p        & 0x1F];			\
	rgba[3] = (p & 0x8000) ? 255 : 0;				\
    } while (0)

#define TAG(x) tdfx##x##_ARGB1555
#include "../dri/common/spantmp.h"


/* 16 bit, RGB565 color spanline and pixel functions */
/* [dBorca] Hack alert:
 * This is wrong. The alpha value is lost, even when we provide
 * HW alpha (565 w/o depth buffering). To really update alpha buffer,
 * we would need to do the 565 writings via 8888 colorformat and rely
 * on the Voodoo to perform color scaling. In which case our 565 span
 * would look nicer! But this violates FSAA rules...
 */

#undef LFB_MODE
#define LFB_MODE	GR_LFBWRITEMODE_565

#undef BYTESPERPIXEL
#define BYTESPERPIXEL 2

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
    p = TDFXPACKCOLOR565( color[RCOMP], color[GCOMP], color[BCOMP] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
    *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch) =			\
					TDFXPACKCOLOR565( r, g, b )

#define WRITE_PIXEL( _x, _y, p )					\
    *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
    do {								\
	GLushort p = *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch);	\
	rgba[0] = FX_rgb_scale_5[(p >> 11) & 0x1F];			\
	rgba[1] = FX_rgb_scale_6[(p >> 5)  & 0x3F];			\
	rgba[2] = FX_rgb_scale_5[ p        & 0x1F];			\
	rgba[3] = 0xff;							\
    } while (0)

#define TAG(x) tdfx##x##_RGB565
#include "../dri/common/spantmp.h"


/* 32 bit, ARGB8888 color spanline and pixel functions */

#undef LFB_MODE
#define LFB_MODE	GR_LFBWRITEMODE_8888

#undef BYTESPERPIXEL
#define BYTESPERPIXEL 4

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
    p = TDFXPACKCOLOR8888( color[RCOMP], color[GCOMP], color[BCOMP], color[ACOMP] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
    *(GLuint *)(buf + _x*BYTESPERPIXEL + _y*pitch) =			\
					TDFXPACKCOLOR8888( r, g, b, a )

#define WRITE_PIXEL( _x, _y, p )					\
    *(GLuint *)(buf + _x*BYTESPERPIXEL + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
    do {								\
	GLuint p = *(GLuint *)(buf + _x*BYTESPERPIXEL + _y*pitch);	\
        rgba[0] = (p >> 16) & 0xff;					\
        rgba[1] = (p >>  8) & 0xff;					\
        rgba[2] = (p >>  0) & 0xff;					\
        rgba[3] = (p >> 24) & 0xff;					\
    } while (0)

#define TAG(x) tdfx##x##_ARGB8888
#include "../dri/common/spantmp.h"


/************************************************************************/
/*****                    Depth functions                           *****/
/************************************************************************/

#define DBG 0

#undef HW_WRITE_LOCK
#undef HW_WRITE_UNLOCK
#undef HW_READ_LOCK
#undef HW_READ_UNLOCK

#define HW_CLIPLOOP HW_WRITE_CLIPLOOP

#define LOCAL_DEPTH_VARS						\
    GLuint pitch = info.strideInBytes;					\
    GLuint height = fxMesa->height;					\
    char *buf = (char *)((char *)info.lfbPtr + 0 /* x, y offset */);	\
    (void) buf;

#define HW_WRITE_LOCK()							\
    fxMesaContext fxMesa = FX_CONTEXT(ctx);				\
    GrLfbInfo_t info;							\
    info.size = sizeof(GrLfbInfo_t);					\
    if ( grLfbLock( GR_LFB_WRITE_ONLY,					\
                   GR_BUFFER_AUXBUFFER, LFB_MODE,			\
		   GR_ORIGIN_UPPER_LEFT, FXFALSE, &info ) ) {

#define HW_WRITE_UNLOCK()						\
	grLfbUnlock( GR_LFB_WRITE_ONLY, GR_BUFFER_AUXBUFFER);		\
    }

#define HW_READ_LOCK()							\
    fxMesaContext fxMesa = FX_CONTEXT(ctx);				\
    GrLfbInfo_t info;							\
    info.size = sizeof(GrLfbInfo_t);					\
    if ( grLfbLock( GR_LFB_READ_ONLY, GR_BUFFER_AUXBUFFER,		\
                    LFB_MODE, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info ) ) {

#define HW_READ_UNLOCK()						\
	grLfbUnlock( GR_LFB_READ_ONLY, GR_BUFFER_AUXBUFFER);		\
    }


/* 16 bit, depth spanline and pixel functions */

#undef LFB_MODE
#define LFB_MODE	GR_LFBWRITEMODE_ZA16

#undef BYTESPERPIXEL
#define BYTESPERPIXEL 2

#define WRITE_DEPTH( _x, _y, d )					\
    *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch) = d

#define READ_DEPTH( d, _x, _y )						\
    d = *(GLushort *)(buf + _x*BYTESPERPIXEL + _y*pitch)

#define TAG(x) tdfx##x##_Z16
#include "../dri/common/depthtmp.h"


/* 24 bit, depth spanline and pixel functions (for use w/ stencil) */
/* [dBorca] Hack alert:
 * This is evil. The incoming Mesa's 24bit depth value
 * is shifted left 8 bits, to obtain a full 32bit value,
 * which will be thrown into the framebuffer. We rely on
 * the fact that Voodoo hardware transforms a 32bit value
 * into 24bit value automatically and, MOST IMPORTANT, won't
 * alter the upper 8bits of the value already existing in the
 * framebuffer (where stencil resides).
 */

#undef LFB_MODE
#define LFB_MODE	GR_LFBWRITEMODE_Z32

#undef BYTESPERPIXEL
#define BYTESPERPIXEL 4

#define WRITE_DEPTH( _x, _y, d )					\
    *(GLuint *)(buf + _x*BYTESPERPIXEL + _y*pitch) = d << 8

#define READ_DEPTH( d, _x, _y )						\
    d = (*(GLuint *)(buf + _x*BYTESPERPIXEL + _y*pitch)) & 0xffffff

#define TAG(x) tdfx##x##_Z24
#include "../dri/common/depthtmp.h"


/* 32 bit, depth spanline and pixel functions (for use w/o stencil) */
/* [dBorca] Hack alert:
 * This is more evil. We make Mesa run in 32bit depth, but
 * tha Voodoo HW can only handle 24bit depth. Well, exploiting
 * the pixel pipeline, we can achieve 24:8 format for greater
 * precision...
 * If anyone tells me how to really store 32bit values into the
 * depth buffer, I'll write the *_Z32 routines. Howver, bear in
 * mind that means running without stencil!
 */

/************************************************************************/
/*****                    Span functions (optimized)                *****/
/************************************************************************/

/*
 * Read a span of 15-bit RGB pixels.  Note, we don't worry about cliprects
 * since OpenGL says obscured pixels have undefined values.
 */
static void fxReadRGBASpan_ARGB1555 (const GLcontext * ctx,
                                     struct gl_renderbuffer *rb,
                                     GLuint n,
                                     GLint x, GLint y,
                                     GLubyte rgba[][4])
{
 fxMesaContext fxMesa = FX_CONTEXT(ctx);
 GrBuffer_t currentFB = GR_BUFFER_BACKBUFFER;
 GrLfbInfo_t info;
 info.size = sizeof(GrLfbInfo_t);
 if (grLfbLock(GR_LFB_READ_ONLY, currentFB,
               GR_LFBWRITEMODE_ANY, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) {
    const GLint winX = 0;
    const GLint winY = fxMesa->height - 1;
    const GLushort *data16 = (const GLushort *)((const GLubyte *)info.lfbPtr +
	                                        (winY - y) * info.strideInBytes +
                                                (winX + x) * 2);
    const GLuint *data32 = (const GLuint *) data16;
    GLuint i, j;
    GLuint extraPixel = (n & 1);
    n -= extraPixel;

    for (i = j = 0; i < n; i += 2, j++) {
	GLuint pixel = data32[j];
	rgba[i][0] = FX_rgb_scale_5[(pixel >> 10) & 0x1F];
	rgba[i][1] = FX_rgb_scale_5[(pixel >> 5)  & 0x1F];
	rgba[i][2] = FX_rgb_scale_5[ pixel        & 0x1F];
	rgba[i][3] = (pixel & 0x8000) ? 255 : 0;
	rgba[i+1][0] = FX_rgb_scale_5[(pixel >> 26) & 0x1F];
	rgba[i+1][1] = FX_rgb_scale_5[(pixel >> 21) & 0x1F];
	rgba[i+1][2] = FX_rgb_scale_5[(pixel >> 16) & 0x1F];
	rgba[i+1][3] = (pixel & 0x80000000) ? 255 : 0;
    }
    if (extraPixel) {
       GLushort pixel = data16[n];
       rgba[n][0] = FX_rgb_scale_5[(pixel >> 10) & 0x1F];
       rgba[n][1] = FX_rgb_scale_5[(pixel >> 5)  & 0x1F];
       rgba[n][2] = FX_rgb_scale_5[ pixel        & 0x1F];
       rgba[n][3] = (pixel & 0x8000) ? 255 : 0;
    }

    grLfbUnlock(GR_LFB_READ_ONLY, currentFB);
 }
}

/*
 * Read a span of 16-bit RGB pixels.  Note, we don't worry about cliprects
 * since OpenGL says obscured pixels have undefined values.
 */
static void fxReadRGBASpan_RGB565 (const GLcontext * ctx,
                                   struct gl_renderbuffer *rb,
                                   GLuint n,
                                   GLint x, GLint y,
                                   GLubyte rgba[][4])
{
 fxMesaContext fxMesa = FX_CONTEXT(ctx);
 GrBuffer_t currentFB = GR_BUFFER_BACKBUFFER;
 GrLfbInfo_t info;
 info.size = sizeof(GrLfbInfo_t);
 if (grLfbLock(GR_LFB_READ_ONLY, currentFB,
               GR_LFBWRITEMODE_ANY, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) {
    const GLint winX = 0;
    const GLint winY = fxMesa->height - 1;
    const GLushort *data16 = (const GLushort *)((const GLubyte *)info.lfbPtr +
	                                        (winY - y) * info.strideInBytes +
                                                (winX + x) * 2);
    const GLuint *data32 = (const GLuint *) data16;
    GLuint i, j;
    GLuint extraPixel = (n & 1);
    n -= extraPixel;

    for (i = j = 0; i < n; i += 2, j++) {
        GLuint pixel = data32[j];
	rgba[i][0] = FX_rgb_scale_5[(pixel >> 11) & 0x1F];
	rgba[i][1] = FX_rgb_scale_6[(pixel >> 5)  & 0x3F];
	rgba[i][2] = FX_rgb_scale_5[ pixel        & 0x1F];
	rgba[i][3] = 255;
	rgba[i+1][0] = FX_rgb_scale_5[(pixel >> 27) & 0x1F];
	rgba[i+1][1] = FX_rgb_scale_6[(pixel >> 21) & 0x3F];
	rgba[i+1][2] = FX_rgb_scale_5[(pixel >> 16) & 0x1F];
	rgba[i+1][3] = 255;
    }
    if (extraPixel) {
       GLushort pixel = data16[n];
       rgba[n][0] = FX_rgb_scale_5[(pixel >> 11) & 0x1F];
       rgba[n][1] = FX_rgb_scale_6[(pixel >> 5)  & 0x3F];
       rgba[n][2] = FX_rgb_scale_5[ pixel        & 0x1F];
       rgba[n][3] = 255;
    }

    grLfbUnlock(GR_LFB_READ_ONLY, currentFB);
 }
}

/*
 * Read a span of 32-bit RGB pixels.  Note, we don't worry about cliprects
 * since OpenGL says obscured pixels have undefined values.
 */
static void fxReadRGBASpan_ARGB8888 (const GLcontext * ctx,
                                     struct gl_renderbuffer *rb,
                                     GLuint n,
                                     GLint x, GLint y,
                                     GLubyte rgba[][4])
{
 fxMesaContext fxMesa = FX_CONTEXT(ctx);
 GrBuffer_t currentFB = GR_BUFFER_BACKBUFFER;
 GLuint i;
 grLfbReadRegion(currentFB, x, fxMesa->height - 1 - y, n, 1, n * 4, rgba);
 for (i = 0; i < n; i++) {
     GLubyte c = rgba[i][0];
     rgba[i][0] = rgba[i][2];
     rgba[i][2] = c;
 }
}


/************************************************************************/
/*****                    Depth functions (optimized)               *****/
/************************************************************************/

static void
fxReadDepthSpan_Z16(GLcontext * ctx, struct gl_renderbuffer *rb,
		    GLuint n, GLint x, GLint y, GLuint depth[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;
   GLushort depth16[MAX_WIDTH];
   GLuint i;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxReadDepthSpan_Z16(...)\n");
   }

   grLfbReadRegion(GR_BUFFER_AUXBUFFER, x, bottom - y, n, 1, 0, depth16);
   for (i = 0; i < n; i++) {
      depth[i] = depth16[i];
   }
}


static void
fxReadDepthSpan_Z24(GLcontext * ctx, struct gl_renderbuffer *rb,
		    GLuint n, GLint x, GLint y, GLuint depth[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;
   GLuint i;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxReadDepthSpan_Z24(...)\n");
   }

   grLfbReadRegion(GR_BUFFER_AUXBUFFER, x, bottom - y, n, 1, 0, depth);
   for (i = 0; i < n; i++) {
      depth[i] &= 0xffffff;
   }
}


/************************************************************************/
/*****                    Stencil functions (optimized)             *****/
/************************************************************************/

static void
fxWriteStencilSpan (GLcontext *ctx, struct gl_renderbuffer *rb,
                    GLuint n, GLint x, GLint y,
                    const GLstencil stencil[], const GLubyte mask[])
{
 /*
  * XXX todo
  */
}

static void
fxReadStencilSpan(GLcontext * ctx, struct gl_renderbuffer *rb,
		  GLuint n, GLint x, GLint y, GLstencil stencil[])
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLint bottom = fxMesa->height - 1;
   GLuint zs32[MAX_WIDTH];
   GLuint i;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxReadStencilSpan(...)\n");
   }

   grLfbReadRegion(GR_BUFFER_AUXBUFFER, x, bottom - y, n, 1, 0, zs32);
   for (i = 0; i < n; i++) {
      stencil[i] = zs32[i] >> 24;
   }
}

static void
fxWriteStencilPixels (GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                      const GLint x[], const GLint y[],
                      const GLstencil stencil[],
                      const GLubyte mask[])
{
 /*
  * XXX todo
  */
}

static void
fxReadStencilPixels (GLcontext *ctx, struct gl_renderbuffer *rb, GLuint n,
                     const GLint x[], const GLint y[],
                     GLstencil stencil[])
{
 /*
  * XXX todo
  */
}


void
fxSetupDDSpanPointers(GLcontext * ctx)
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference( ctx );
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   switch (fxMesa->colDepth) {
          case 15:
               swdd->WriteRGBASpan = tdfxWriteRGBASpan_ARGB1555;
               swdd->WriteRGBSpan = tdfxWriteRGBSpan_ARGB1555;
               swdd->WriteRGBAPixels = tdfxWriteRGBAPixels_ARGB1555;
               swdd->WriteMonoRGBASpan = tdfxWriteMonoRGBASpan_ARGB1555;
               swdd->WriteMonoRGBAPixels = tdfxWriteMonoRGBAPixels_ARGB1555;
               swdd->ReadRGBASpan = /*td*/fxReadRGBASpan_ARGB1555;
               swdd->ReadRGBAPixels = tdfxReadRGBAPixels_ARGB1555;

               swdd->WriteDepthSpan = tdfxWriteDepthSpan_Z16;
               swdd->WriteDepthPixels = tdfxWriteDepthPixels_Z16;
               swdd->ReadDepthSpan = /*td*/fxReadDepthSpan_Z16;
               swdd->ReadDepthPixels = tdfxReadDepthPixels_Z16;
               break;
          case 16:
               swdd->WriteRGBASpan = tdfxWriteRGBASpan_RGB565;
               swdd->WriteRGBSpan = tdfxWriteRGBSpan_RGB565;
               swdd->WriteRGBAPixels = tdfxWriteRGBAPixels_RGB565;
               swdd->WriteMonoRGBASpan = tdfxWriteMonoRGBASpan_RGB565;
               swdd->WriteMonoRGBAPixels = tdfxWriteMonoRGBAPixels_RGB565;
               swdd->ReadRGBASpan = /*td*/fxReadRGBASpan_RGB565;
               swdd->ReadRGBAPixels = tdfxReadRGBAPixels_RGB565;

               swdd->WriteDepthSpan = tdfxWriteDepthSpan_Z16;
               swdd->WriteDepthPixels = tdfxWriteDepthPixels_Z16;
               swdd->ReadDepthSpan = /*td*/fxReadDepthSpan_Z16;
               swdd->ReadDepthPixels = tdfxReadDepthPixels_Z16;
               break;
          case 32:
               swdd->WriteRGBASpan = tdfxWriteRGBASpan_ARGB8888;
               swdd->WriteRGBSpan = tdfxWriteRGBSpan_ARGB8888;
               swdd->WriteRGBAPixels = tdfxWriteRGBAPixels_ARGB8888;
               swdd->WriteMonoRGBASpan = tdfxWriteMonoRGBASpan_ARGB8888;
               swdd->WriteMonoRGBAPixels = tdfxWriteMonoRGBAPixels_ARGB8888;
               swdd->ReadRGBASpan = /*td*/fxReadRGBASpan_ARGB8888;
               swdd->ReadRGBAPixels = tdfxReadRGBAPixels_ARGB8888;

               swdd->WriteDepthSpan = tdfxWriteDepthSpan_Z24;
               swdd->WriteDepthPixels = tdfxWriteDepthPixels_Z24;
               swdd->ReadDepthSpan = /*td*/fxReadDepthSpan_Z24;
               swdd->ReadDepthPixels = tdfxReadDepthPixels_Z24;
               break;
   }

   if (fxMesa->haveHwStencil) {
      swdd->WriteStencilSpan = fxWriteStencilSpan;
      swdd->ReadStencilSpan = fxReadStencilSpan;
      swdd->WriteStencilPixels = fxWriteStencilPixels;
      swdd->ReadStencilPixels = fxReadStencilPixels;
   }
#if 0
   swdd->WriteCI8Span		= NULL;
   swdd->WriteCI32Span		= NULL;
   swdd->WriteMonoCISpan	= NULL;
   swdd->WriteCI32Pixels	= NULL;
   swdd->WriteMonoCIPixels	= NULL;
   swdd->ReadCI32Span		= NULL;
   swdd->ReadCI32Pixels		= NULL;

   swdd->SpanRenderStart        = tdfxSpanRenderStart; /* BEGIN_BOARD_LOCK */
   swdd->SpanRenderFinish       = tdfxSpanRenderFinish; /* END_BOARD_LOCK */
#endif
}


#else


/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_span(void);
int
gl_fx_dummy_function_span(void)
{
   return 0;
}

#endif /* FX */
