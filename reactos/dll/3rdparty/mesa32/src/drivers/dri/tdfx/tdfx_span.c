/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_span.c,v 1.7 2002/10/30 12:52:00 alanh Exp $ */

/*
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Brian Paul <brianp@valinux.com>
 *	Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#include "tdfx_context.h"
#include "tdfx_lock.h"
#include "tdfx_span.h"
#include "tdfx_render.h"
#include "swrast/swrast.h"


#define DBG 0


#define LOCAL_VARS							\
   driRenderbuffer *drb = (driRenderbuffer *) rb;			\
   __DRIdrawablePrivate *const dPriv = drb->dPriv;			\
   GLuint pitch = drb->backBuffer ? info.strideInBytes			\
     : (drb->pitch * drb->cpp);						\
   const GLuint bottom = dPriv->h - 1;					\
   char *buf = (char *)((char *)info.lfbPtr +				\
			 (dPriv->x * drb->cpp) +			\
			 (dPriv->y * pitch));				\
   GLuint p;								\
   (void) buf; (void) p;


#define Y_FLIP(_y)		(bottom - _y)


#define HW_WRITE_LOCK()							\
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);				\
   GrLfbInfo_t info;							\
   FLUSH_BATCH( fxMesa );						\
   UNLOCK_HARDWARE( fxMesa );						\
   LOCK_HARDWARE( fxMesa );						\
   info.size = sizeof(GrLfbInfo_t);					\
   if (fxMesa->Glide.grLfbLock(GR_LFB_WRITE_ONLY, fxMesa->DrawBuffer,	\
			       LFB_MODE, GR_ORIGIN_UPPER_LEFT, FXFALSE,	\
			       &info)) {

#define HW_WRITE_UNLOCK()						\
      fxMesa->Glide.grLfbUnlock( GR_LFB_WRITE_ONLY, fxMesa->DrawBuffer );\
   }


#define HW_READ_LOCK()							\
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);				\
   GrLfbInfo_t info;							\
   FLUSH_BATCH( fxMesa );						\
   UNLOCK_HARDWARE( fxMesa );						\
   LOCK_HARDWARE( fxMesa );						\
   info.size = sizeof(GrLfbInfo_t);					\
   if ( fxMesa->Glide.grLfbLock( GR_LFB_READ_ONLY, fxMesa->ReadBuffer,	\
                   LFB_MODE, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info ) )	\
   {

#define HW_READ_UNLOCK()						\
      fxMesa->Glide.grLfbUnlock( GR_LFB_READ_ONLY, fxMesa->ReadBuffer );\
   }


#define HW_WRITE_CLIPLOOP()						\
      do {								\
         int _nc = fxMesa->numClipRects;				\
         while (_nc--) {						\
            int minx = fxMesa->pClipRects[_nc].x1 - fxMesa->x_offset;	\
	    int miny = fxMesa->pClipRects[_nc].y1 - fxMesa->y_offset;	\
	    int maxx = fxMesa->pClipRects[_nc].x2 - fxMesa->x_offset;	\
	    int maxy = fxMesa->pClipRects[_nc].y2 - fxMesa->y_offset;

#define HW_READ_CLIPLOOP()						\
      do {								\
         const __DRIdrawablePrivate *dPriv = fxMesa->driDrawable;	\
         drm_clip_rect_t *rect = dPriv->pClipRects;			\
         int _nc = dPriv->numClipRects;					\
         while (_nc--) {						\
            const int minx = rect->x1 - fxMesa->x_offset;		\
            const int miny = rect->y1 - fxMesa->y_offset;		\
            const int maxx = rect->x2 - fxMesa->x_offset;		\
            const int maxy = rect->y2 - fxMesa->y_offset;		\
            rect++;

#define HW_ENDCLIPLOOP()						\
	 }								\
      } while (0)



#define LFB_MODE	GR_LFBWRITEMODE_565


/* 16 bit, RGB565 color spanline and pixel functions */			\

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = TDFXPACKCOLOR565( color[0], color[1], color[2] )


#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch) = ((((int)r & 0xf8) << 8) |	\
					   (((int)g & 0xfc) << 3) |	\
					   (((int)b & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )					\
    *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
    do {								\
	GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);		\
	rgba[0] = (((p >> 11) & 0x1f) * 255) / 31;			\
	rgba[1] = (((p >>  5) & 0x3f) * 255) / 63;			\
	rgba[2] = (((p >>  0) & 0x1f) * 255) / 31;			\
	rgba[3] = 0xff;							\
    } while (0)

#define TAG(x) tdfx##x##_RGB565
#define BYTESPERPIXEL 2
#include "spantmp.h"
#undef BYTESPERPIXEL


/* 16 bit, BGR565 color spanline and pixel functions */			\
#if 0

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch) = ((((int)b & 0xf8) << 8) |	\
					   (((int)g & 0xfc) << 3) |	\
					   (((int)r & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )					\
    *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
    do {								\
	GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);		\
	rgba[0] = (p << 3) & 0xf8;					\
	rgba[1] = (p >> 3) & 0xfc;					\
	rgba[2] = (p >> 8) & 0xf8;					\
	rgba[3] = 0xff;							\
    } while (0)

#define TAG(x) tdfx##x##_BGR565
#define BYTESPERPIXEL 2
#include "spantmp.h"
#undef BYTESPERPIXEL
#endif


#undef LFB_MODE
#define LFB_MODE	GR_LFBWRITEMODE_888


/* 24 bit, RGB888 color spanline and pixel functions */
#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = TDFXPACKCOLOR888( color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLuint *)(buf + _x*3 + _y*pitch) = ((b << 0) |			\
					 (g << 8) |			\
					 (r << 16))

#define WRITE_PIXEL( _x, _y, p )					\
   *(GLuint *)(buf + _x*3 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
do {									\
   GLuint p = *(GLuint *)(buf + _x*3 + _y*pitch);			\
   rgba[0] = (p >> 16) & 0xff;						\
   rgba[1] = (p >> 8)  & 0xff;						\
   rgba[2] = (p >> 0)  & 0xff;						\
   rgba[3] = 0xff;							\
} while (0)

#define TAG(x) tdfx##x##_RGB888
#define BYTESPERPIXEL 4
#include "spantmp.h"
#undef BYTESPERPIXEL


#undef LFB_MODE
#define LFB_MODE	GR_LFBWRITEMODE_8888


/* 32 bit, ARGB8888 color spanline and pixel functions */
#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = TDFXPACKCOLOR8888( color[0], color[1], color[2], color[3] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLuint *)(buf + _x*4 + _y*pitch) = ((b <<  0) |			\
					 (g <<  8) |			\
					 (r << 16) |			\
					 (a << 24) )

#define WRITE_PIXEL( _x, _y, p )					\
   *(GLuint *)(buf + _x*4 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
do {									\
   GLuint p = *(GLuint *)(buf + _x*4 + _y*pitch);			\
   rgba[0] = (p >> 16) & 0xff;						\
   rgba[1] = (p >>  8) & 0xff;						\
   rgba[2] = (p >>  0) & 0xff;						\
   rgba[3] = (p >> 24) & 0xff;						\
} while (0)

#define TAG(x) tdfx##x##_ARGB8888
#define BYTESPERPIXEL 4
#include "spantmp.h"
#undef BYTESPERPIXEL



/* ================================================================
 * Old span functions below...
 */


/*
 * Examine the cliprects to generate an array of flags to indicate
 * which pixels in a span are visible.  Note: (x,y) is a screen
 * coordinate.
 */
static void
generate_vismask(const tdfxContextPtr fxMesa, GLint x, GLint y, GLint n,
                 GLubyte vismask[])
{
   GLboolean initialized = GL_FALSE;
   GLint i, j;

   /* Ensure we clear the visual mask */
   MEMSET(vismask, 0, n);

   /* turn on flags for all visible pixels */
   for (i = 0; i < fxMesa->numClipRects; i++) {
      const drm_clip_rect_t *rect = &fxMesa->pClipRects[i];

      if (y >= rect->y1 && y < rect->y2) {
	 if (x >= rect->x1 && x + n <= rect->x2) {
	    /* common case, whole span inside cliprect */
	    MEMSET(vismask, 1, n);
	    return;
	 }
	 if (x < rect->x2 && x + n >= rect->x1) {
	    /* some of the span is inside the rect */
	    GLint start, end;
	    if (!initialized) {
	       MEMSET(vismask, 0, n);
	       initialized = GL_TRUE;
	    }
	    if (x < rect->x1)
	       start = rect->x1 - x;
	    else
	       start = 0;
	    if (x + n > rect->x2)
	       end = rect->x2 - x;
	    else
	       end = n;
	    assert(start >= 0);
	    assert(end <= n);
	    for (j = start; j < end; j++)
	       vismask[j] = 1;
	 }
      }
   }
}

/*
 * Examine cliprects and determine if the given screen pixel is visible.
 */
static GLboolean
visible_pixel(const tdfxContextPtr fxMesa, int scrX, int scrY)
{
   int i;
   for (i = 0; i < fxMesa->numClipRects; i++) {
      const drm_clip_rect_t *rect = &fxMesa->pClipRects[i];
      if (scrX >= rect->x1 &&
	  scrX < rect->x2 &&
	  scrY >= rect->y1 && scrY < rect->y2) return GL_TRUE;
   }
   return GL_FALSE;
}



/*
 * Depth buffer read/write functions.
 */
/*
 * To read the frame buffer, we need to lock and unlock it.  The
 * four macros {READ,WRITE}_FB_SPAN_{LOCK,UNLOCK}
 * do this for us.
 *
 * Note that the lock must be matched with an unlock.  These
 * macros include a spare curly brace, so they must
 * be syntactically matched.
 *
 * Note, also, that you can't lock a buffer twice with different
 * modes.  That is to say, you can't lock a buffer in both read
 * and write modes.  The strideInBytes and LFB pointer will be
 * the same with read and write locks, so you can use either.
 * o The HW has different state for reads and writes, so
 *   locking it twice may give screwy results.
 * o The DRM won't let you lock twice.  It hangs.  This is probably
 *   because of the LOCK_HARDWARE IN THE *_FB_SPAN_LOCK macros,
 *   and could be eliminated with nonlocking lock routines.  But
 *   what's the point after all.
 */
#define READ_FB_SPAN_LOCK(fxMesa, info, target_buffer)              \
  UNLOCK_HARDWARE(fxMesa);                                          \
  LOCK_HARDWARE(fxMesa);                                            \
  (info).size=sizeof(info);                                         \
  if (fxMesa->Glide.grLfbLock(GR_LFB_READ_ONLY,                     \
                target_buffer,                                      \
                GR_LFBWRITEMODE_ANY,                                \
                GR_ORIGIN_UPPER_LEFT,                               \
                FXFALSE,                                            \
                &(info))) {

#define READ_FB_SPAN_UNLOCK(fxMesa, target_buffer)                  \
    fxMesa->Glide.grLfbUnlock(GR_LFB_READ_ONLY, target_buffer);     \
  } else {                                                          \
    fprintf(stderr, "tdfxDriver: Can't get %s (%d) read lock\n",    \
            (target_buffer == GR_BUFFER_BACKBUFFER)                 \
                ? "back buffer"                                     \
            : ((target_buffer == GR_BUFFER_AUXBUFFER)               \
                ? "depth buffer"                                    \
               : "unknown buffer"),                                 \
            target_buffer);                                         \
  }

#define WRITE_FB_SPAN_LOCK(fxMesa, info, target_buffer, write_mode) \
  UNLOCK_HARDWARE(fxMesa);                                          \
  LOCK_HARDWARE(fxMesa);                                            \
  info.size=sizeof(info);                                           \
  if (fxMesa->Glide.grLfbLock(GR_LFB_WRITE_ONLY,                    \
                target_buffer,                                      \
                write_mode,                                         \
                GR_ORIGIN_UPPER_LEFT,                               \
                FXFALSE,                                            \
                &info)) {

#define WRITE_FB_SPAN_UNLOCK(fxMesa, target_buffer)                 \
    fxMesa->Glide.grLfbUnlock(GR_LFB_WRITE_ONLY, target_buffer);    \
  } else {                                                          \
    fprintf(stderr, "tdfxDriver: Can't get %s (%d) write lock\n",   \
            (target_buffer == GR_BUFFER_BACKBUFFER)                 \
                ? "back buffer"                                     \
            : ((target_buffer == GR_BUFFER_AUXBUFFER)               \
                ? "depth buffer"                                    \
               : "unknown buffer"),                                 \
            target_buffer);                                         \
  }

/*
 * Because the Linear Frame Buffer is not necessarily aligned
 * with the depth buffer, we have to do some fiddling
 * around to get the right addresses.
 *
 * Perhaps a picture is in order.  The Linear Frame Buffer
 * looks like this:
 *
 *   |<----------------------info.strideInBytes------------->|
 *   |<-----physicalStrideInBytes------->|
 *   +-----------------------------------+xxxxxxxxxxxxxxxxxxx+
 *   |                                   |                   |
 *   |          Legal Memory             |  Forbidden Zone   |
 *   |                                   |                   |
 *   +-----------------------------------+xxxxxxxxxxxxxxxxxxx+
 *
 * You can only reliably read and write legal locations.  Reads
 * and writes from the Forbidden Zone will return undefined values,
 * and may cause segmentation faults.
 *
 * Now, the depth buffer may not end up in a location such each
 * scan line is an LFB line.  For example, the depth buffer may
 * look like this:
 *
 *    wrapped               ordinary.
 *   +-----------------------------------+xxxxxxxxxxxxxxxxxxx+
 *   |0000000000000000000000             |                   | back
 *   |1111111111111111111111             |                   | buffer
 *   |2222222222222222222222             |                   |
 *   |4096b align. padxx00000000000000000|  Forbidden Zone   | depth
 *   |0000              11111111111111111|                   | buffer
 *   |1111              22222222222222222|                   |
 *   |2222                               |                   |
 *   +-----------------------------------+xxxxxxxxxxxxxxxxxxx+
 * where each number is the scan line number.  We know it will
 * be aligned on 128 byte boundaries, at least.  Aligning this
 * on a scanline boundary causes the back and depth buffers to
 * thrash in the SST1 cache.  (Note that the back buffer is always
 * allocated at the beginning of LFB memory, and so it is always
 * properly aligned with the LFB stride.)
 *
 * We call the beginning of the line (which is the rightmost
 * part of the depth line in the picture above) the *ordinary* part
 * of the scanline, and the end of the line (which is the
 * leftmost part, one line below) the *wrapped* part of the scanline.
 * a.) We need to know what x value to subtract from the screen
 *     x coordinate to index into the wrapped part.
 * b.) We also need to figure out if we need to read from the ordinary
 *     part scan line, or from the wrapped part of the scan line.
 *
 * [ad a]
 * The first wrapped x coordinate is that coordinate such that
 *           depthBufferOffset&(info.strideInBytes) + x*elmentSize  {*}
 *                            > physicalStrideInBytes
 *     where depthBufferOffset is the LFB distance in bytes
 *     from the back buffer to the depth buffer.  The expression
 *           depthBufferOffset&(info.strideInBytes)
 *     is then the offset (in bytes) from the beginining of (any)
 *     depth buffer line to first element in the line.
 * Simplifying inequation {*} above we see that x is the smallest
 * value such that
 *         x*elementSize > physicalStrideInBytes                      {**}
 *                            - depthBufferOffset&(info.strideInBytes)
 * Now, we know that both the summands on the right are multiples of
 * 128, and elementSize <= 4, so if equality holds in {**}, x would
 * be a multiple of 32.  Thus we can set x to
 *         xwrapped = (physicalStrideInBytes
 *                      - depthBufferOffset&(info.strideInBytes))/elementSize
 *                      + 1
 *
 * [ad b]
 * Question b is now simple.  We read from the wrapped scan line if
 * x is greater than xwrapped.
 */
#define TILE_WIDTH_IN_BYTES		128
#define TILE_WIDTH_IN_ZOXELS(bpz)	(TILE_WIDTH_IN_BYTES/(bpz))
#define TILE_HEIGHT_IN_LINES		32
typedef struct
{
   void *lfbPtr;
   void *lfbWrapPtr;
   FxU32 LFBStrideInElts;
   GLint firstWrappedX;
}
LFBParameters;

/*
 * We need information about the back buffer.  Note that
 * this function *cannot be called* while the aux buffer
 * is locked, or the caller will hang.
 *
 * Only Glide knows the LFB address of the back and depth
 * offsets.  The upper levels of Mesa know the depth offset,
 * but that is not in LFB space, it is tiled memory space,
 * and is not useable for us.
 */
static void
GetBackBufferInfo(tdfxContextPtr fxMesa, GrLfbInfo_t * backBufferInfo)
{
   READ_FB_SPAN_LOCK(fxMesa, *backBufferInfo, GR_BUFFER_BACKBUFFER);
   READ_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_BACKBUFFER);
}

static void
GetFbParams(tdfxContextPtr fxMesa,
            GrLfbInfo_t * info,
            GrLfbInfo_t * backBufferInfo,
            LFBParameters * ReadParamsp, FxU32 elementSize)
{
   FxU32 physicalStrideInBytes, bufferOffset;
   FxU32 strideInBytes = info->strideInBytes;
   char *lfbPtr = (char *) (info->lfbPtr); /* For arithmetic, use char * */

   /*
    * These two come directly from the info structure.
    */
   ReadParamsp->lfbPtr = (void *) lfbPtr;
   ReadParamsp->LFBStrideInElts = strideInBytes / elementSize;
   /*
    * Now, calculate the value of firstWrappedX.
    *
    * The physical stride is the screen width in bytes rounded up to
    * the next highest multiple of 128 bytes.  Note that this fails
    * when TILE_WIDTH_IN_BYTES is not a power of two.
    *
    * The buffer Offset is the distance between the beginning of
    * the LFB space, which is the beginning of the back buffer,
    * and the buffer we are gathering information about.
    * We want to make this routine usable for operations on the
    * back buffer, though we don't actually use it on the back
    * buffer.  Note, then, that if bufferOffset == 0, the firstWrappedX
    * is in the forbidden zone, and is therefore never reached.
    *
    * Note that if
    *     physicalStrideInBytes
    *             < bufferOffset&(info->strideInBytes-1)
    * the buffer begins in the forbidden zone.  We assert for this.
    */
   bufferOffset = (FxU32)(lfbPtr - (char *) backBufferInfo->lfbPtr);
   physicalStrideInBytes
      = (fxMesa->screen_width * elementSize + TILE_WIDTH_IN_BYTES - 1)
      & ~(TILE_WIDTH_IN_BYTES - 1);
   assert(physicalStrideInBytes > (bufferOffset & (strideInBytes - 1)));
   ReadParamsp->firstWrappedX
      = (physicalStrideInBytes
	 - (bufferOffset & (strideInBytes - 1))) / elementSize;
   /*
    * This is the address of the next physical line.
    */
   ReadParamsp->lfbWrapPtr
      = (void *) ((char *) backBufferInfo->lfbPtr
		  + (bufferOffset & ~(strideInBytes - 1))
		  + (TILE_HEIGHT_IN_LINES) * strideInBytes);
}

/*
 * These macros fetch data from the frame buffer.  The type is
 * the type of data we want to fetch.  It should match the type
 * whose size was used with GetFbParams to fill in the structure
 * in *ReadParamsp.  We have a macro to read the ordinary
 * part, a second macro to read the wrapped part, and one which
 * will do either.  When we are reading a span, we will know
 * when the ordinary part ends, so there's no need to test for
 * it.  However, when reading and writing pixels, we don't
 * necessarily know.  I suppose it's a matter of taste whether
 * it's better in the macro or in the call.
 *
 * Recall that x and y are screen coordinates.
 */
#define GET_ORDINARY_FB_DATA(ReadParamsp, type, x, y)               \
    (((type *)((ReadParamsp)->lfbPtr))                              \
                 [(y) * ((ReadParamsp)->LFBStrideInElts)            \
                   + (x)])
#define GET_WRAPPED_FB_DATA(ReadParamsp, type, x, y)                \
    (((type *)((ReadParamsp)->lfbWrapPtr))                          \
                 [((y)) * ((ReadParamsp)->LFBStrideInElts)          \
                   + ((x) - (ReadParamsp)->firstWrappedX)])
#define GET_FB_DATA(ReadParamsp, type, x, y)                        \
   (((x) < (ReadParamsp)->firstWrappedX)                            \
        ? GET_ORDINARY_FB_DATA(ReadParamsp, type, x, y)             \
        : GET_WRAPPED_FB_DATA(ReadParamsp, type, x, y))
#define PUT_ORDINARY_FB_DATA(ReadParamsp, type, x, y, value)              \
    (GET_ORDINARY_FB_DATA(ReadParamsp, type, x, y) = (type)(value))
#define PUT_WRAPPED_FB_DATA(ReadParamsp, type, x, y, value)                \
    (GET_WRAPPED_FB_DATA(ReadParamsp, type, x, y) = (type)(value))
#define PUT_FB_DATA(ReadParamsp, type, x, y, value)                 \
    do {                                                            \
        if ((x) < (ReadParamsp)->firstWrappedX)                     \
            PUT_ORDINARY_FB_DATA(ReadParamsp, type, x, y, value);   \
        else                                                        \
            PUT_WRAPPED_FB_DATA(ReadParamsp, type, x, y, value);    \
    } while (0)


static void
tdfxDDWriteDepthSpan(GLcontext * ctx, struct gl_renderbuffer *rb,
		     GLuint n, GLint x, GLint y, const void *values,
		     const GLubyte mask[])
{
   const GLuint *depth = (const GLuint *) values;
   tdfxContextPtr fxMesa = (tdfxContextPtr) ctx->DriverCtx;
   GLint bottom = fxMesa->y_offset + fxMesa->height - 1;
   GLuint depth_size = fxMesa->glCtx->Visual.depthBits;
   GLuint stencil_size = fxMesa->glCtx->Visual.stencilBits;
   GrLfbInfo_t info;
   GLubyte visMask[MAX_WIDTH];

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "tdfxmesa: tdfxDDWriteDepthSpan(...)\n");
   }

   assert((depth_size == 16) || (depth_size == 24) || (depth_size == 32));
   /*
    * Convert x and y to screen coordinates.
    */
   x += fxMesa->x_offset;
   y = bottom - y;
   if (mask) {
      GLint i;
      GLushort d16;
      GrLfbInfo_t backBufferInfo;

      switch (depth_size) {
      case 16:
	 GetBackBufferInfo(fxMesa, &backBufferInfo);
	 /*
	  * Note that the _LOCK macro adds a curly brace,
	  * and the UNLOCK macro removes it.
	  */
	 WRITE_FB_SPAN_LOCK(fxMesa, info, GR_BUFFER_AUXBUFFER,
			    GR_LFBWRITEMODE_ANY);
	 generate_vismask(fxMesa, x, y, n, visMask);
	 {
	    LFBParameters ReadParams;
	    int wrappedPartStart;
	    GetFbParams(fxMesa, &info, &backBufferInfo,
			&ReadParams, sizeof(GLushort));
	    if (ReadParams.firstWrappedX <= x) {
	       wrappedPartStart = 0;
	    }
	    else if (n <= (ReadParams.firstWrappedX - x)) {
	       wrappedPartStart = n;
	    }
	    else {
	       wrappedPartStart = (ReadParams.firstWrappedX - x);
	    }
	    for (i = 0; i < wrappedPartStart; i++) {
	       if (mask[i] && visMask[i]) {
		  d16 = depth[i];
		  PUT_ORDINARY_FB_DATA(&ReadParams, GLushort, x + i, y, d16);
	       }
	    }
	    for (; i < n; i++) {
	       if (mask[i] && visMask[i]) {
		  d16 = depth[i];
		  PUT_WRAPPED_FB_DATA(&ReadParams, GLushort, x + i, y, d16);
	       }
	    }
	 }
	 WRITE_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
	 break;
      case 24:
      case 32:
	 GetBackBufferInfo(fxMesa, &backBufferInfo);
	 /*
	  * Note that the _LOCK macro adds a curly brace,
	  * and the UNLOCK macro removes it.
	  */
	 WRITE_FB_SPAN_LOCK(fxMesa, info, GR_BUFFER_AUXBUFFER,
			    GR_LFBWRITEMODE_ANY);
	 generate_vismask(fxMesa, x, y, n, visMask);
	 {
	    LFBParameters ReadParams;
	    int wrappedPartStart;
	    GetFbParams(fxMesa, &info, &backBufferInfo,
			&ReadParams, sizeof(GLuint));
	    if (ReadParams.firstWrappedX <= x) {
	       wrappedPartStart = 0;
	    }
	    else if (n <= (ReadParams.firstWrappedX - x)) {
	       wrappedPartStart = n;
	    }
	    else {
	       wrappedPartStart = (ReadParams.firstWrappedX - x);
	    }
	    for (i = 0; i < wrappedPartStart; i++) {
	       GLuint d32;
	       if (mask[i] && visMask[i]) {
		  if (stencil_size > 0) {
		     d32 =
			GET_ORDINARY_FB_DATA(&ReadParams, GLuint,
					     x + i, y);
		     d32 =
			(d32 & 0xFF000000) | (depth[i] & 0x00FFFFFF);
		  }
		  else {
		     d32 = depth[i];
		  }
		  PUT_ORDINARY_FB_DATA(&ReadParams, GLuint, x + i, y, d32);
	       }
	    }
	    for (; i < n; i++) {
	       GLuint d32;
	       if (mask[i] && visMask[i]) {
		  if (stencil_size > 0) {
		     d32 =
			GET_WRAPPED_FB_DATA(&ReadParams, GLuint,
					    x + i, y);
		     d32 =
			(d32 & 0xFF000000) | (depth[i] & 0x00FFFFFF);
		  }
		  else {
		     d32 = depth[i];
		  }
		  PUT_WRAPPED_FB_DATA(&ReadParams, GLuint, x + i, y, d32);
	       }
	    }
	 }
	 WRITE_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
	 break;
      }
   }
   else {
      GLint i;
      GLuint d32;
      GLushort d16;
      GrLfbInfo_t backBufferInfo;

      switch (depth_size) {
      case 16:
	 GetBackBufferInfo(fxMesa, &backBufferInfo);
	 /*
	  * Note that the _LOCK macro adds a curly brace,
	  * and the UNLOCK macro removes it.
	  */
	 WRITE_FB_SPAN_LOCK(fxMesa, info,
			    GR_BUFFER_AUXBUFFER, GR_LFBWRITEMODE_ANY);
	 generate_vismask(fxMesa, x, y, n, visMask);
	 {
	    LFBParameters ReadParams;
	    GLuint wrappedPartStart;
	    GetFbParams(fxMesa, &info, &backBufferInfo,
			&ReadParams, sizeof(GLushort));
	    if (ReadParams.firstWrappedX <= x) {
	       wrappedPartStart = 0;
	    }
	    else if (n <= (ReadParams.firstWrappedX - x)) {
	       wrappedPartStart = n;
	    }
	    else {
	       wrappedPartStart = (ReadParams.firstWrappedX - x);
	    }
	    for (i = 0; i < wrappedPartStart; i++) {
	       if (visMask[i]) {
		  d16 = depth[i];
		  PUT_ORDINARY_FB_DATA(&ReadParams,
				       GLushort,
				       x + i, y,
				       d16);
	       }
	    }
	    for (; i < n; i++) {
	       if (visMask[i]) {
		  d16 = depth[i];
		  PUT_WRAPPED_FB_DATA(&ReadParams,
				      GLushort,
				      x + i, y,
				      d16);
	       }
	    }
	 }
	 WRITE_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
	 break;
      case 24:
      case 32:
	 GetBackBufferInfo(fxMesa, &backBufferInfo);
	 /*
	  * Note that the _LOCK macro adds a curly brace,
	  * and the UNLOCK macro removes it.
	  */
	 WRITE_FB_SPAN_LOCK(fxMesa, info,
			    GR_BUFFER_AUXBUFFER, GR_LFBWRITEMODE_ANY);
	 generate_vismask(fxMesa, x, y, n, visMask);
	 {
	    LFBParameters ReadParams;
	    GLuint wrappedPartStart;

	    GetFbParams(fxMesa, &info, &backBufferInfo,
			&ReadParams, sizeof(GLuint));
	    if (ReadParams.firstWrappedX <= x) {
	       wrappedPartStart = 0;
	    }
	    else if (n <= (ReadParams.firstWrappedX - x)) {
	       wrappedPartStart = n;
	    }
	    else {
	       wrappedPartStart = (ReadParams.firstWrappedX - x);
	    }
	    for (i = 0; i < wrappedPartStart; i++) {
	       if (visMask[i]) {
		  if (stencil_size > 0) {
		     d32 = GET_ORDINARY_FB_DATA(&ReadParams, GLuint, x + i, y);
		     d32 =
			(d32 & 0xFF000000) | (depth[i] & 0x00FFFFFF);
		  }
		  else {
		     d32 = depth[i];
		  }
		  PUT_ORDINARY_FB_DATA(&ReadParams, GLuint, x + i, y, d32);
	       }
	    }
	    for (; i < n; i++) {
	       if (visMask[i]) {
		  if (stencil_size > 0) {
		     d32 = GET_WRAPPED_FB_DATA(&ReadParams, GLuint, x + i, y);
		     d32 =
			(d32 & 0xFF000000) | (depth[i] & 0x00FFFFFF);
		  }
		  else {
		     d32 = depth[i];
		  }
		  PUT_WRAPPED_FB_DATA(&ReadParams, GLuint, x + i, y, d32);
	       }
	    }
	 }
	 WRITE_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
	 break;
      }
   }
}

static void
tdfxDDWriteMonoDepthSpan(GLcontext * ctx, struct gl_renderbuffer *rb,
                         GLuint n, GLint x, GLint y, const void *value,
                         const GLubyte mask[])
{
   GLuint depthVal = *((GLuint *) value);
   GLuint depths[MAX_WIDTH];
   GLuint i;
   for (i = 0; i < n; i++)
      depths[i] = depthVal;
   tdfxDDWriteDepthSpan(ctx, rb, n, x, y, depths, mask);
}


static void
tdfxDDReadDepthSpan(GLcontext * ctx, struct gl_renderbuffer *rb,
		    GLuint n, GLint x, GLint y, void *values)
{
   GLuint *depth = (GLuint *) values;
   tdfxContextPtr fxMesa = (tdfxContextPtr) ctx->DriverCtx;
   GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
   GLuint i;
   GLuint depth_size = fxMesa->glCtx->Visual.depthBits;
   GrLfbInfo_t info;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "tdfxmesa: tdfxDDReadDepthSpan(...)\n");
   }

   /*
    * Convert to screen coordinates.
    */
   x += fxMesa->x_offset;
   y = bottom - y;
   switch (depth_size) {
   case 16:
   {
      LFBParameters ReadParams;
      GrLfbInfo_t backBufferInfo;
      int wrappedPartStart;
      GetBackBufferInfo(fxMesa, &backBufferInfo);
      /*
       * Note that the _LOCK macro adds a curly brace,
       * and the UNLOCK macro removes it.
       */
      READ_FB_SPAN_LOCK(fxMesa, info, GR_BUFFER_AUXBUFFER);
      GetFbParams(fxMesa, &info, &backBufferInfo,
		  &ReadParams, sizeof(GLushort));
      if (ReadParams.firstWrappedX <= x) {
	 wrappedPartStart = 0;
      }
      else if (n <= (ReadParams.firstWrappedX - x)) {
	 wrappedPartStart = n;
      }
      else {
	 wrappedPartStart = (ReadParams.firstWrappedX - x);
      }
      /*
       * Read the line.
       */
      for (i = 0; i < wrappedPartStart; i++) {
	 depth[i] =
	    GET_ORDINARY_FB_DATA(&ReadParams, GLushort, x + i, y);
      }
      for (; i < n; i++) {
	 depth[i] = GET_WRAPPED_FB_DATA(&ReadParams, GLushort,
					x + i, y);
      }
      READ_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
      break;
   }
   case 24:
   case 32:
   {
      LFBParameters ReadParams;
      GrLfbInfo_t backBufferInfo;
      int wrappedPartStart;
      GLuint stencil_size = fxMesa->glCtx->Visual.stencilBits;
      GetBackBufferInfo(fxMesa, &backBufferInfo);
      /*
       * Note that the _LOCK macro adds a curly brace,
       * and the UNLOCK macro removes it.
       */
      READ_FB_SPAN_LOCK(fxMesa, info, GR_BUFFER_AUXBUFFER);
      GetFbParams(fxMesa, &info, &backBufferInfo,
		  &ReadParams, sizeof(GLuint));
      if (ReadParams.firstWrappedX <= x) {
	 wrappedPartStart = 0;
      }
      else if (n <= (ReadParams.firstWrappedX - x)) {
	 wrappedPartStart = n;
      }
      else {
	 wrappedPartStart = (ReadParams.firstWrappedX - x);
      }
      /*
       * Read the line.
       */
      for (i = 0; i < wrappedPartStart; i++) {
	 const GLuint mask =
	    (stencil_size > 0) ? 0x00FFFFFF : 0xFFFFFFFF;
	 depth[i] =
	    GET_ORDINARY_FB_DATA(&ReadParams, GLuint, x + i, y);
	 depth[i] &= mask;
      }
      for (; i < n; i++) {
	 const GLuint mask =
	    (stencil_size > 0) ? 0x00FFFFFF : 0xFFFFFFFF;
	 depth[i] = GET_WRAPPED_FB_DATA(&ReadParams, GLuint, x + i, y);
	 depth[i] &= mask;
      }
      READ_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
      break;
   }
   }
}


static void
tdfxDDWriteDepthPixels(GLcontext * ctx, struct gl_renderbuffer *rb,
		       GLuint n, const GLint x[], const GLint y[],
		       const void *values, const GLubyte mask[])
{
   const GLuint *depth = (const GLuint *) values;
   tdfxContextPtr fxMesa = (tdfxContextPtr) ctx->DriverCtx;
   GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
   GLuint i;
   GLushort d16;
   GLuint d32;
   GLuint depth_size = fxMesa->glCtx->Visual.depthBits;
   GLuint stencil_size = fxMesa->glCtx->Visual.stencilBits;
   GrLfbInfo_t info;
   int xpos;
   int ypos;
   GrLfbInfo_t backBufferInfo;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "tdfxmesa: tdfxDDWriteDepthPixels(...)\n");
   }

   switch (depth_size) {
   case 16:
      GetBackBufferInfo(fxMesa, &backBufferInfo);
      /*
       * Note that the _LOCK macro adds a curly brace,
       * and the UNLOCK macro removes it.
       */
      WRITE_FB_SPAN_LOCK(fxMesa, info,
			 GR_BUFFER_AUXBUFFER, GR_LFBWRITEMODE_ANY);
      {
	 LFBParameters ReadParams;
	 GetFbParams(fxMesa, &info, &backBufferInfo,
		     &ReadParams, sizeof(GLushort));
	 for (i = 0; i < n; i++) {
	    if ((!mask || mask[i]) && visible_pixel(fxMesa, x[i], y[i])) {
	       xpos = x[i] + fxMesa->x_offset;
	       ypos = bottom - y[i];
	       d16 = depth[i];
	       PUT_FB_DATA(&ReadParams, GLushort, xpos, ypos, d16);
	    }
	 }
      }
      WRITE_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
      break;
   case 24:
   case 32:
      GetBackBufferInfo(fxMesa, &backBufferInfo);
      /*
       * Note that the _LOCK macro adds a curly brace,
       * and the UNLOCK macro removes it.
       */
      WRITE_FB_SPAN_LOCK(fxMesa, info,
			 GR_BUFFER_AUXBUFFER, GR_LFBWRITEMODE_ANY);
      {
	 LFBParameters ReadParams;
	 GetFbParams(fxMesa, &info, &backBufferInfo,
		     &ReadParams, sizeof(GLuint));
	 for (i = 0; i < n; i++) {
	    if (!mask || mask[i]) {
	       if (visible_pixel(fxMesa, x[i], y[i])) {
		  xpos = x[i] + fxMesa->x_offset;
		  ypos = bottom - y[i];
		  if (stencil_size > 0) {
		     d32 =
			GET_FB_DATA(&ReadParams, GLuint, xpos, ypos);
		     d32 = (d32 & 0xFF000000) | (depth[i] & 0xFFFFFF);
		  }
		  else {
		     d32 = depth[i];
		  }
		  PUT_FB_DATA(&ReadParams, GLuint, xpos, ypos, d32);
	       }
	    }
	 }
      }
      WRITE_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
      break;
   }
}


static void
tdfxDDReadDepthPixels(GLcontext * ctx, struct gl_renderbuffer *rb, GLuint n,
		      const GLint x[], const GLint y[], void *values)
{
   GLuint *depth = (GLuint *) values;
   tdfxContextPtr fxMesa = (tdfxContextPtr) ctx->DriverCtx;
   GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
   GLuint i;
   GLuint depth_size = fxMesa->glCtx->Visual.depthBits;
   GLushort d16;
   int xpos;
   int ypos;
   GrLfbInfo_t info;
   GLuint stencil_size;
   GrLfbInfo_t backBufferInfo;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "tdfxmesa: tdfxDDReadDepthPixels(...)\n");
   }

   assert((depth_size == 16) || (depth_size == 24) || (depth_size == 32));
   switch (depth_size) {
   case 16:
      GetBackBufferInfo(fxMesa, &backBufferInfo);
      /*
       * Note that the _LOCK macro adds a curly brace,
       * and the UNLOCK macro removes it.
       */
      READ_FB_SPAN_LOCK(fxMesa, info, GR_BUFFER_AUXBUFFER);
      {
	 LFBParameters ReadParams;
	 GetFbParams(fxMesa, &info, &backBufferInfo,
		     &ReadParams, sizeof(GLushort));
	 for (i = 0; i < n; i++) {
	    /*
	     * Convert to screen coordinates.
	     */
	    xpos = x[i] + fxMesa->x_offset;
	    ypos = bottom - y[i];
	    d16 = GET_FB_DATA(&ReadParams, GLushort, xpos, ypos);
	    depth[i] = d16;
	 }
      }
      READ_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
      break;
   case 24:
   case 32:
      GetBackBufferInfo(fxMesa, &backBufferInfo);
      /*
       * Note that the _LOCK macro adds a curly brace,
       * and the UNLOCK macro removes it.
       */
      READ_FB_SPAN_LOCK(fxMesa, info, GR_BUFFER_AUXBUFFER);
      stencil_size = fxMesa->glCtx->Visual.stencilBits;
      {
	 LFBParameters ReadParams;
	 GetFbParams(fxMesa, &info, &backBufferInfo,
		     &ReadParams, sizeof(GLuint));
	 for (i = 0; i < n; i++) {
	    GLuint d32;

	    /*
	     * Convert to screen coordinates.
	     */
	    xpos = x[i] + fxMesa->x_offset;
	    ypos = bottom - y[i];
	    d32 = GET_FB_DATA(&ReadParams, GLuint, xpos, ypos);
	    if (stencil_size > 0) {
	       d32 &= 0x00FFFFFF;
	    }
	    depth[i] = d32;
	 }
      }
      READ_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
      break;
   default:
      assert(0);
   }
}

/*
 * Stencil buffer read/write functions.
 */
#define EXTRACT_S_FROM_ZS(zs) (((zs) >> 24) & 0xFF)
#define EXTRACT_Z_FROM_ZS(zs) ((zs) & 0xffffff)
#define BUILD_ZS(z, s)  (((s) << 24) | (z))

static void
write_stencil_span(GLcontext * ctx, struct gl_renderbuffer *rb,
                   GLuint n, GLint x, GLint y,
                   const void *values, const GLubyte mask[])
{
   const GLubyte *stencil = (const GLubyte *) values;
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GrLfbInfo_t info;
   GrLfbInfo_t backBufferInfo;

   GetBackBufferInfo(fxMesa, &backBufferInfo);
   /*
    * Note that the _LOCK macro adds a curly brace,
    * and the UNLOCK macro removes it.
    */
   WRITE_FB_SPAN_LOCK(fxMesa, info, GR_BUFFER_AUXBUFFER, GR_LFBWRITEMODE_ANY);
   {
      const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
      const GLint winX = fxMesa->x_offset;
      const GLint scrX = winX + x;
      const GLint scrY = winY - y;
      LFBParameters ReadParams;
      GLubyte visMask[MAX_WIDTH];
      GLuint i;
      int wrappedPartStart;

      GetFbParams(fxMesa, &info, &backBufferInfo, &ReadParams,
		  sizeof(GLuint));
      if (ReadParams.firstWrappedX <= x) {
	 wrappedPartStart = 0;
      }
      else if (n <= (ReadParams.firstWrappedX - x)) {
	 wrappedPartStart = n;
      }
      else {
	 wrappedPartStart = (ReadParams.firstWrappedX - x);
      }
      generate_vismask(fxMesa, scrX, scrY, n, visMask);
      for (i = 0; i < wrappedPartStart; i++) {
	 if (visMask[i] && (!mask || mask[i])) {
	    GLuint z = GET_ORDINARY_FB_DATA(&ReadParams, GLuint,
					    scrX + i, scrY) & 0x00FFFFFF;
	    z |= (stencil[i] & 0xFF) << 24;
	    PUT_ORDINARY_FB_DATA(&ReadParams, GLuint, scrX + i, scrY, z);
	 }
      }
      for (; i < n; i++) {
	 if (visMask[i] && (!mask || mask[i])) {
	    GLuint z = GET_WRAPPED_FB_DATA(&ReadParams, GLuint,
					   scrX + i, scrY) & 0x00FFFFFF;
	    z |= (stencil[i] & 0xFF) << 24;
	    PUT_WRAPPED_FB_DATA(&ReadParams, GLuint, scrX + i, scrY, z);
	 }
      }
   }
   WRITE_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
}


static void
write_mono_stencil_span(GLcontext * ctx, struct gl_renderbuffer *rb,
                        GLuint n, GLint x, GLint y,
                        const void *value, const GLubyte mask[])
{
   GLbyte stencilVal = *((GLbyte *) value);
   GLbyte stencils[MAX_WIDTH];
   GLuint i;
   for (i = 0; i < n; i++)
      stencils[i] = stencilVal;
   write_stencil_span(ctx, rb, n, x, y, stencils, mask);
}


static void
read_stencil_span(GLcontext * ctx, struct gl_renderbuffer *rb,
                  GLuint n, GLint x, GLint y,
                  void *values)
{
   GLubyte *stencil = (GLubyte *) values;
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GrLfbInfo_t info;
   GrLfbInfo_t backBufferInfo;

   GetBackBufferInfo(fxMesa, &backBufferInfo);
   /*
    * Note that the _LOCK macro adds a curly brace,
    * and the UNLOCK macro removes it.
    */
   READ_FB_SPAN_LOCK(fxMesa, info, GR_BUFFER_AUXBUFFER);
   {
      const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
      const GLint winX = fxMesa->x_offset;
      GLuint i;
      LFBParameters ReadParams;
      int wrappedPartStart;

      /*
       * Convert to screen coordinates.
       */
      x += winX;
      y = winY - y;
      GetFbParams(fxMesa, &info, &backBufferInfo, &ReadParams,
		  sizeof(GLuint));
      if (ReadParams.firstWrappedX <= x) {
	 wrappedPartStart = 0;
      }
      else if (n <= (ReadParams.firstWrappedX - x)) {
	 wrappedPartStart = n;
      }
      else {
	 wrappedPartStart = (ReadParams.firstWrappedX - x);
      }
      for (i = 0; i < wrappedPartStart; i++) {
	 stencil[i] = (GET_ORDINARY_FB_DATA(&ReadParams, GLuint,
					    x + i, y) >> 24) & 0xFF;
      }
      for (; i < n; i++) {
	 stencil[i] = (GET_WRAPPED_FB_DATA(&ReadParams, GLuint,
					   x + i, y) >> 24) & 0xFF;
      }
   }
   READ_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
}


static void
write_stencil_pixels(GLcontext * ctx, struct gl_renderbuffer *rb,
                     GLuint n, const GLint x[], const GLint y[],
                     const void *values, const GLubyte mask[])
{
   const GLubyte *stencil = (const GLubyte *) values;
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GrLfbInfo_t info;
   GrLfbInfo_t backBufferInfo;

   GetBackBufferInfo(fxMesa, &backBufferInfo);
   /*
    * Note that the _LOCK macro adds a curly brace,
    * and the UNLOCK macro removes it.
    */
   WRITE_FB_SPAN_LOCK(fxMesa, info, GR_BUFFER_AUXBUFFER, GR_LFBWRITEMODE_ANY);
   {
      const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
      const GLint winX = fxMesa->x_offset;
      LFBParameters ReadParams;
      GLuint i;

      GetFbParams(fxMesa, &info, &backBufferInfo, &ReadParams,
		  sizeof(GLuint));
      for (i = 0; i < n; i++) {
	 const GLint scrX = winX + x[i];
	 const GLint scrY = winY - y[i];
	 if ((!mask || mask[i]) && visible_pixel(fxMesa, scrX, scrY)) {
	    GLuint z =
	       GET_FB_DATA(&ReadParams, GLuint, scrX, scrY) & 0x00FFFFFF;
	    z |= (stencil[i] & 0xFF) << 24;
	    PUT_FB_DATA(&ReadParams, GLuint, scrX, scrY, z);
	 }
      }
   }
   WRITE_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
}


static void
read_stencil_pixels(GLcontext * ctx, struct gl_renderbuffer *rb,
                    GLuint n, const GLint x[], const GLint y[],
                    void *values)
{
   GLubyte *stencil = (GLubyte *) values;
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GrLfbInfo_t info;
   GrLfbInfo_t backBufferInfo;

   GetBackBufferInfo(fxMesa, &backBufferInfo);
   /*
    * Note that the _LOCK macro adds a curly brace,
    * and the UNLOCK macro removes it.
    */
   READ_FB_SPAN_LOCK(fxMesa, info, GR_BUFFER_AUXBUFFER);
   {
      const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
      const GLint winX = fxMesa->x_offset;
      GLuint i;
      LFBParameters ReadParams;

      GetFbParams(fxMesa, &info, &backBufferInfo, &ReadParams,
		  sizeof(GLuint));
      for (i = 0; i < n; i++) {
	 const GLint scrX = winX + x[i];
	 const GLint scrY = winY - y[i];
	 stencil[i] =
	    (GET_FB_DATA(&ReadParams, GLuint, scrX, scrY) >> 24) & 0xFF;
      }
   }
   READ_FB_SPAN_UNLOCK(fxMesa, GR_BUFFER_AUXBUFFER);
}

#define VISUAL_EQUALS_RGBA(vis, r, g, b, a)        \
   ((vis.redBits == r) &&                         \
    (vis.greenBits == g) &&                       \
    (vis.blueBits == b) &&                        \
    (vis.alphaBits == a))




/**********************************************************************/
/*                    Locking for swrast                              */
/**********************************************************************/


static void tdfxSpanRenderStart( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   LOCK_HARDWARE(fxMesa);
}

static void tdfxSpanRenderFinish( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   _swrast_flush( ctx );
   UNLOCK_HARDWARE(fxMesa);
}

/**********************************************************************/
/*                    Initialize swrast device driver                 */
/**********************************************************************/

void tdfxDDInitSpanFuncs( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference( ctx );
   swdd->SpanRenderStart          = tdfxSpanRenderStart;
   swdd->SpanRenderFinish         = tdfxSpanRenderFinish; 
}



/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
tdfxSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         tdfxInitPointers_RGB565(&drb->Base);
      }
      else if (vis->redBits == 8 && vis->greenBits == 8
               && vis->blueBits == 8 && vis->alphaBits == 0) {
         tdfxInitPointers_RGB888(&drb->Base);
      }
      else if (vis->redBits == 8 && vis->greenBits == 8
               && vis->blueBits == 8 && vis->alphaBits == 8) {
         tdfxInitPointers_ARGB8888(&drb->Base);
      }
      else {
         _mesa_problem(NULL, "problem in tdfxSetSpanFunctions");
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16 ||
            drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      drb->Base.GetRow        = tdfxDDReadDepthSpan;
      drb->Base.GetValues     = tdfxDDReadDepthPixels;
      drb->Base.PutRow        = tdfxDDWriteDepthSpan;
      drb->Base.PutMonoRow    = tdfxDDWriteMonoDepthSpan;
      drb->Base.PutValues     = tdfxDDWriteDepthPixels;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      drb->Base.GetRow        = read_stencil_span;
      drb->Base.GetValues     = read_stencil_pixels;
      drb->Base.PutRow        = write_stencil_span;
      drb->Base.PutMonoRow    = write_mono_stencil_span;
      drb->Base.PutValues     = write_stencil_pixels;
      drb->Base.PutMonoValues = NULL;
   }
}
