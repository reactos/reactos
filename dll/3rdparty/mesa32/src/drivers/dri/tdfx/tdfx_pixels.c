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
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_pixels.c,v 1.4 2002/02/22 21:45:03 dawes Exp $ */

/*
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Brian Paul <brianp@valinux.com>
 *	Nathan Hand <nhand@valinux.com>
 *
 */

#include "tdfx_context.h"
#include "tdfx_dd.h"
#include "tdfx_lock.h"
#include "tdfx_vb.h"
#include "tdfx_pixels.h"
#include "tdfx_render.h"

#include "swrast/swrast.h"

#include "image.h"


#define FX_grLfbWriteRegion(fxMesa,dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)		\
  do {				\
    LOCK_HARDWARE(fxMesa);		\
    fxMesa->Glide.grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,FXFALSE,src_stride,src_data);	\
    UNLOCK_HARDWARE(fxMesa);		\
  } while(0)


#define FX_grLfbReadRegion(fxMesa,src_buffer,src_x,src_y,src_width,src_height,dst_stride,dst_data)			\
  do {				\
    LOCK_HARDWARE(fxMesa);		\
    fxMesa->Glide.grLfbReadRegion(src_buffer,src_x,src_y,src_width,src_height,dst_stride,dst_data);				\
    UNLOCK_HARDWARE(fxMesa);		\
  } while (0);


#if 0
static FxBool
FX_grLfbLock(tdfxContextPtr fxMesa, GrLock_t type, GrBuffer_t buffer,
             GrLfbWriteMode_t writeMode, GrOriginLocation_t origin,
             FxBool pixelPipeline, GrLfbInfo_t * info)
{
   FxBool result;

   LOCK_HARDWARE(fxMesa);
   result = fxMesa->Glide.grLfbLock(type, buffer, writeMode, origin, pixelPipeline, info);
   UNLOCK_HARDWARE(fxMesa);
   return result;
}
#endif


#define FX_grLfbUnlock(fxMesa, t, b)	\
  do {					\
    LOCK_HARDWARE(fxMesa);		\
    fxMesa->Glide.grLfbUnlock(t, b);	\
    UNLOCK_HARDWARE(fxMesa);		\
  } while (0)



#if 0
/* test if window coord (px,py) is visible */
static GLboolean
inClipRects(tdfxContextPtr fxMesa, int px, int py)
{
    int i;
    for (i = 0; i < fxMesa->numClipRects; i++) {
        if ((px >= fxMesa->pClipRects[i].x1) &&
            (px < fxMesa->pClipRects[i].x2) &&
            (py >= fxMesa->pClipRects[i].y1) &&
            (py < fxMesa->pClipRects[i].y2)) return GL_TRUE;
    }
    return GL_FALSE;
}
#endif

/* test if rectangle of pixels (px,py) (px+width,py+height) is visible */
static GLboolean
inClipRects_Region(tdfxContextPtr fxMesa, int x, int y, int width, int height)
{
    int i;
    int x1, y1, x2, y2;
    int xmin, xmax, ymin, ymax, pixelsleft;

    y1 = y - height + 1; y2 = y;
    x1 = x; x2 = x + width - 1;
    pixelsleft = width * height;

    for (i = 0; i < fxMesa->numClipRects; i++)
    {
        /* algorithm requires x1 < x2 and y1 < y2 */
        if ((fxMesa->pClipRects[i].x1 < fxMesa->pClipRects[i].x2)) {
            xmin = fxMesa->pClipRects[i].x1;
            xmax = fxMesa->pClipRects[i].x2-1;
        } else {
            xmin = fxMesa->pClipRects[i].x2;
            xmax = fxMesa->pClipRects[i].x1-1;
        }
        if ((fxMesa->pClipRects[i].y1 < fxMesa->pClipRects[i].y2)) {
            ymin = fxMesa->pClipRects[i].y1;
            ymax = fxMesa->pClipRects[i].y2-1;
        } else {
            ymin = fxMesa->pClipRects[i].y2;
            ymax = fxMesa->pClipRects[i].y1-1;
        }

        /* reject trivial cases */
        if (xmax < x1) continue;
        if (ymax < y1) continue;
        if (xmin > x2) continue;
        if (ymin > y2) continue;

        /* find the intersection */
        if (xmin < x1) xmin = x1;
        if (ymin < y1) ymin = y1;
        if (xmax > x2) xmax = x2;
        if (ymax > y2) ymax = y2;

        pixelsleft -= (xmax-xmin+1) * (ymax-ymin+1);
    }

    return pixelsleft == 0;
}

#if 0
GLboolean
tdfx_bitmap_R5G6B5(GLcontext * ctx, GLint px, GLint py,
		   GLsizei width, GLsizei height,
		   const struct gl_pixelstore_attrib *unpack,
		   const GLubyte * bitmap)
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GrLfbInfo_t info;
   TdfxU16 color;
   const struct gl_pixelstore_attrib *finalUnpack;
   struct gl_pixelstore_attrib scissoredUnpack;

   /* check if there's any raster operations enabled which we can't handle */
   if (ctx->RasterMask & (ALPHATEST_BIT |
			  BLEND_BIT |
			  DEPTH_BIT |
			  FOG_BIT |
			  LOGIC_OP_BIT |
			  SCISSOR_BIT |
			  STENCIL_BIT |
			  MASKING_BIT |
			  MULTI_DRAW_BIT)) return GL_FALSE;

   if (ctx->Scissor.Enabled) {
      /* This is a bit tricky, but by carefully adjusting the px, py,
       * width, height, skipPixels and skipRows values we can do
       * scissoring without special code in the rendering loop.
       */

      /* we'll construct a new pixelstore struct */
      finalUnpack = &scissoredUnpack;
      scissoredUnpack = *unpack;
      if (scissoredUnpack.RowLength == 0)
	 scissoredUnpack.RowLength = width;

      /* clip left */
      if (px < ctx->Scissor.X) {
	 scissoredUnpack.SkipPixels += (ctx->Scissor.X - px);
	 width -= (ctx->Scissor.X - px);
	 px = ctx->Scissor.X;
      }
      /* clip right */
      if (px + width >= ctx->Scissor.X + ctx->Scissor.Width) {
	 width -= (px + width - (ctx->Scissor.X + ctx->Scissor.Width));
      }
      /* clip bottom */
      if (py < ctx->Scissor.Y) {
	 scissoredUnpack.SkipRows += (ctx->Scissor.Y - py);
	 height -= (ctx->Scissor.Y - py);
	 py = ctx->Scissor.Y;
      }
      /* clip top */
      if (py + height >= ctx->Scissor.Y + ctx->Scissor.Height) {
	 height -= (py + height - (ctx->Scissor.Y + ctx->Scissor.Height));
      }

      if (width <= 0 || height <= 0)
	 return GL_TRUE;     /* totally scissored away */
   }
   else {
      finalUnpack = unpack;
   }

   /* compute pixel value */
   {
      GLint r = (GLint) (ctx->Current.RasterColor[0] * 255.0f);
      GLint g = (GLint) (ctx->Current.RasterColor[1] * 255.0f);
      GLint b = (GLint) (ctx->Current.RasterColor[2] * 255.0f);
      /*GLint a = (GLint)(ctx->Current.RasterColor[3]*255.0f); */
      if (fxMesa->bgrOrder) {
	 color = (TdfxU16)
	    (((TdfxU16) 0xf8 & b) << (11 - 3)) |
	    (((TdfxU16) 0xfc & g) << (5 - 3 + 1)) |
	    (((TdfxU16) 0xf8 & r) >> 3);
      }
      else
	 color = (TdfxU16)
	    (((TdfxU16) 0xf8 & r) << (11 - 3)) |
	    (((TdfxU16) 0xfc & g) << (5 - 3 + 1)) |
	    (((TdfxU16) 0xf8 & b) >> 3);
   }

   info.size = sizeof(info);
   if (!TDFX_grLfbLock(fxMesa,
		     GR_LFB_WRITE_ONLY,
		     fxMesa->currentFB,
		     GR_LFBWRITEMODE_565,
		     GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) {
#ifndef TDFX_SILENT
      fprintf(stderr, "tdfx Driver: error locking the linear frame buffer\n");
#endif
      return GL_TRUE;
   }

   {
      const GLint winX = fxMesa->x_offset;
      const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
      /* The dest stride depends on the hardware and whether we're drawing
       * to the front or back buffer.  This compile-time test seems to do
       * the job for now.
       */
      const GLint dstStride = (fxMesa->glCtx->Color.DrawBuffer[0] == GL_FRONT)
	 ? (fxMesa->screen_width) : (info.strideInBytes / 2);
      GLint row;
      /* compute dest address of bottom-left pixel in bitmap */
      GLushort *dst = (GLushort *) info.lfbPtr
	 + (winY - py) * dstStride + (winX + px);

      for (row = 0; row < height; row++) {
	 const GLubyte *src =
	    (const GLubyte *) _mesa_image_address2d(finalUnpack,
                                                    bitmap, width, height,
                                                    GL_COLOR_INDEX,
                                                    GL_BITMAP, row, 0);
	 if (finalUnpack->LsbFirst) {
	    /* least significan bit first */
	    GLubyte mask = 1U << (finalUnpack->SkipPixels & 0x7);
	    GLint col;
	    for (col = 0; col < width; col++) {
	       if (*src & mask) {
		  if (inClipRects(fxMesa, winX + px + col, winY - py - row))
		     dst[col] = color;
	       }
	       if (mask == 128U) {
		  src++;
		  mask = 1U;
	       }
	       else {
		  mask = mask << 1;
	       }
	    }
	    if (mask != 1)
	       src++;
	 }
	 else {
	    /* most significan bit first */
	    GLubyte mask = 128U >> (finalUnpack->SkipPixels & 0x7);
	    GLint col;
	    for (col = 0; col < width; col++) {
	       if (*src & mask) {
		  if (inClipRects(fxMesa, winX + px + col, winY - py - row))
		     dst[col] = color;
	       }
	       if (mask == 1U) {
		  src++;
		  mask = 128U;
	       }
	       else {
		  mask = mask >> 1;
	       }
	    }
	    if (mask != 128)
	       src++;
	 }
	 dst -= dstStride;
      }
   }

   TDFX_grLfbUnlock(fxMesa, GR_LFB_WRITE_ONLY, fxMesa->currentFB);
   return GL_TRUE;
}
#endif

#if 0
GLboolean
tdfx_bitmap_R8G8B8A8(GLcontext * ctx, GLint px, GLint py,
		     GLsizei width, GLsizei height,
		     const struct gl_pixelstore_attrib *unpack,
		     const GLubyte * bitmap)
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GrLfbInfo_t info;
   GLuint color;
   const struct gl_pixelstore_attrib *finalUnpack;
   struct gl_pixelstore_attrib scissoredUnpack;

   /* check if there's any raster operations enabled which we can't handle */
   if (ctx->RasterMask & (ALPHATEST_BIT |
			  BLEND_BIT |
			  DEPTH_BIT |
			  FOG_BIT |
			  LOGIC_OP_BIT |
			  SCISSOR_BIT |
			  STENCIL_BIT |
			  MASKING_BIT |
			  MULTI_DRAW_BIT)) return GL_FALSE;

   if (ctx->Scissor.Enabled) {
      /* This is a bit tricky, but by carefully adjusting the px, py,
       * width, height, skipPixels and skipRows values we can do
       * scissoring without special code in the rendering loop.
       */

      /* we'll construct a new pixelstore struct */
      finalUnpack = &scissoredUnpack;
      scissoredUnpack = *unpack;
      if (scissoredUnpack.RowLength == 0)
	 scissoredUnpack.RowLength = width;

      /* clip left */
      if (px < ctx->Scissor.X) {
	 scissoredUnpack.SkipPixels += (ctx->Scissor.X - px);
	 width -= (ctx->Scissor.X - px);
	 px = ctx->Scissor.X;
      }
      /* clip right */
      if (px + width >= ctx->Scissor.X + ctx->Scissor.Width) {
	 width -= (px + width - (ctx->Scissor.X + ctx->Scissor.Width));
      }
      /* clip bottom */
      if (py < ctx->Scissor.Y) {
	 scissoredUnpack.SkipRows += (ctx->Scissor.Y - py);
	 height -= (ctx->Scissor.Y - py);
	 py = ctx->Scissor.Y;
      }
      /* clip top */
      if (py + height >= ctx->Scissor.Y + ctx->Scissor.Height) {
	 height -= (py + height - (ctx->Scissor.Y + ctx->Scissor.Height));
      }

      if (width <= 0 || height <= 0)
	 return GL_TRUE;     /* totally scissored away */
   }
   else {
      finalUnpack = unpack;
   }

   /* compute pixel value */
   {
      GLint r = (GLint) (ctx->Current.RasterColor[0] * 255.0f);
      GLint g = (GLint) (ctx->Current.RasterColor[1] * 255.0f);
      GLint b = (GLint) (ctx->Current.RasterColor[2] * 255.0f);
      GLint a = (GLint) (ctx->Current.RasterColor[3] * 255.0f);
      color = PACK_BGRA32(r, g, b, a);
   }

   info.size = sizeof(info);
   if (!TDFX_grLfbLock(fxMesa, GR_LFB_WRITE_ONLY,
		     fxMesa->currentFB, GR_LFBWRITEMODE_8888,
		     GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) {
#ifndef TDFX_SILENT
      fprintf(stderr, "tdfx Driver: error locking the linear frame buffer\n");
#endif
      return GL_TRUE;
   }

   {
      const GLint winX = fxMesa->x_offset;
      const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
      GLint dstStride;
      GLuint *dst;
      GLint row;

      if (fxMesa->glCtx->Color.DrawBuffer[0] == GL_FRONT) {
	 dstStride = fxMesa->screen_width;
	 dst =
	    (GLuint *) info.lfbPtr + (winY - py) * dstStride + (winX +
								px);
      }
      else {
	 dstStride = info.strideInBytes / 4;
	 dst =
	    (GLuint *) info.lfbPtr + (winY - py) * dstStride + (winX +
								px);
      }

      /* compute dest address of bottom-left pixel in bitmap */
      for (row = 0; row < height; row++) {
	 const GLubyte *src =
	    (const GLubyte *) _mesa_image_address2d(finalUnpack,
                                                    bitmap, width, height,
                                                    GL_COLOR_INDEX,
                                                    GL_BITMAP, row, 0);
	 if (finalUnpack->LsbFirst) {
	    /* least significan bit first */
	    GLubyte mask = 1U << (finalUnpack->SkipPixels & 0x7);
	    GLint col;
	    for (col = 0; col < width; col++) {
	       if (*src & mask) {
		  if (inClipRects(fxMesa, winX + px + col, winY - py - row))
		     dst[col] = color;
	       }
	       if (mask == 128U) {
		  src++;
		  mask = 1U;
	       }
	       else {
		  mask = mask << 1;
	       }
	    }
	    if (mask != 1)
	       src++;
	 }
	 else {
	    /* most significan bit first */
	    GLubyte mask = 128U >> (finalUnpack->SkipPixels & 0x7);
	    GLint col;
	    for (col = 0; col < width; col++) {
	       if (*src & mask) {
		  if (inClipRects(fxMesa, winX + px + col, winY - py - row))
		     dst[col] = color;
	       }
	       if (mask == 1U) {
		  src++;
		  mask = 128U;
	       }
	       else {
		  mask = mask >> 1;
	       }
	    }
	    if (mask != 128)
	       src++;
	 }
	 dst -= dstStride;
      }
   }

   TDFX_grLfbUnlock(fxMesa, GR_LFB_WRITE_ONLY, fxMesa->currentFB);
   return GL_TRUE;
}
#endif

void
tdfx_readpixels_R5G6B5(GLcontext * ctx, GLint x, GLint y,
		       GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       const struct gl_pixelstore_attrib *packing,
		       GLvoid * dstImage)
{
   if (format != GL_RGB ||
       type != GL_UNSIGNED_SHORT_5_6_5 ||
       (ctx->_ImageTransferState & (IMAGE_SCALE_BIAS_BIT|
				    IMAGE_MAP_COLOR_BIT)))
   {
      _swrast_ReadPixels( ctx, x, y, width, height, format, type, packing,
			  dstImage );
      return;
   }

   {
      tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
      GrLfbInfo_t info;
      __DRIdrawablePrivate *const readable = fxMesa->driReadable;
      const GLint winX = readable->x;
      const GLint winY = readable->y + readable->h - 1;
      const GLint scrX = winX + x;
      const GLint scrY = winY - y;

      LOCK_HARDWARE( fxMesa );
      info.size = sizeof(info);
      if (fxMesa->Glide.grLfbLock(GR_LFB_READ_ONLY,
		    fxMesa->ReadBuffer,
		    GR_LFBWRITEMODE_ANY,
		    GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) {
	 const GLint srcStride = (fxMesa->glCtx->Color.DrawBuffer[0] ==
	     GL_FRONT) ? (fxMesa->screen_width) : (info.strideInBytes / 2);
	 const GLushort *src = (const GLushort *) info.lfbPtr
	    + scrY * srcStride + scrX;
	 GLubyte *dst = (GLubyte *) _mesa_image_address2d(packing,
            dstImage, width, height, format, type, 0, 0);
	 const GLint dstStride = _mesa_image_row_stride(packing,
            width, format, type);

	 /* directly memcpy 5R6G5B pixels into client's buffer */
	 const GLint widthInBytes = width * 2;
	 GLint row;
	 for (row = 0; row < height; row++) {
	    MEMCPY(dst, src, widthInBytes);
	    dst += dstStride;
	    src -= srcStride;
	 }

	 fxMesa->Glide.grLfbUnlock(GR_LFB_READ_ONLY, fxMesa->ReadBuffer);
      }
      UNLOCK_HARDWARE( fxMesa );
      return;
   }
}

void
tdfx_readpixels_R8G8B8A8(GLcontext * ctx, GLint x, GLint y,
                         GLsizei width, GLsizei height,
                         GLenum format, GLenum type,
                         const struct gl_pixelstore_attrib *packing,
                         GLvoid * dstImage)
{
   if ((!(format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8) &&
	!(format == GL_BGRA && type == GL_UNSIGNED_BYTE)) ||
       (ctx->_ImageTransferState & (IMAGE_SCALE_BIAS_BIT|
				    IMAGE_MAP_COLOR_BIT)))
   {
      _swrast_ReadPixels( ctx, x, y, width, height, format, type, packing,
			  dstImage );
      return;
   }


   {
      tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
      GrLfbInfo_t info;
      __DRIdrawablePrivate *const readable = fxMesa->driReadable;
      const GLint winX = readable->x;
      const GLint winY = readable->y + readable->h - 1;
      const GLint scrX = winX + x;
      const GLint scrY = winY - y;

      LOCK_HARDWARE(fxMesa);
      info.size = sizeof(info);
      if (fxMesa->Glide.grLfbLock(GR_LFB_READ_ONLY,
                    fxMesa->ReadBuffer,
                    GR_LFBWRITEMODE_ANY,
                    GR_ORIGIN_UPPER_LEFT, FXFALSE, &info))
      {
         const GLint srcStride = (fxMesa->glCtx->Color.DrawBuffer[0] == GL_FRONT)
            ? (fxMesa->screen_width) : (info.strideInBytes / 4);
         const GLuint *src = (const GLuint *) info.lfbPtr
            + scrY * srcStride + scrX;
         const GLint dstStride =
            _mesa_image_row_stride(packing, width, format, type);
         GLubyte *dst = (GLubyte *) _mesa_image_address2d(packing,
            dstImage, width, height, format, type, 0, 0);
         const GLint widthInBytes = width * 4;

	 {
            GLint row;
            for (row = 0; row < height; row++) {
               MEMCPY(dst, src, widthInBytes);
               dst += dstStride;
               src -= srcStride;
            }
         }

         fxMesa->Glide.grLfbUnlock(GR_LFB_READ_ONLY, fxMesa->ReadBuffer);
      }
      UNLOCK_HARDWARE(fxMesa);
   }
}

void
tdfx_drawpixels_R8G8B8A8(GLcontext * ctx, GLint x, GLint y,
                         GLsizei width, GLsizei height,
                         GLenum format, GLenum type,
                         const struct gl_pixelstore_attrib *unpack,
                         const GLvoid * pixels)
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   if ((!(format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8) &&
	!(format == GL_BGRA && type == GL_UNSIGNED_BYTE)) ||
       ctx->Pixel.ZoomX != 1.0F || 
       ctx->Pixel.ZoomY != 1.0F ||
       (ctx->_ImageTransferState & (IMAGE_SCALE_BIAS_BIT|
				    IMAGE_MAP_COLOR_BIT)) ||
       ctx->Color.AlphaEnabled ||
       ctx->Depth.Test ||
       ctx->Fog.Enabled ||
       ctx->Scissor.Enabled ||
       ctx->Stencil.Enabled ||
       !ctx->Color.ColorMask[0] ||
       !ctx->Color.ColorMask[1] ||
       !ctx->Color.ColorMask[2] ||
       !ctx->Color.ColorMask[3] ||
       ctx->Color.ColorLogicOpEnabled ||
       ctx->Texture._EnabledUnits ||
       fxMesa->Fallback)       
   {
      _swrast_DrawPixels( ctx, x, y, width, height, format, type, 
			  unpack, pixels );
      return; 
   }

   {
      tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
      GrLfbInfo_t info;
      GLboolean result = GL_FALSE;

      const GLint winX = fxMesa->x_offset;
      const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
      const GLint scrX = winX + x;
      const GLint scrY = winY - y;

      /* lock early to make sure cliprects are right */
      LOCK_HARDWARE(fxMesa);

      /* make sure hardware has latest blend funcs */
      if (ctx->Color.BlendEnabled) {
         fxMesa->dirty |= TDFX_UPLOAD_BLEND_FUNC;
         tdfxEmitHwStateLocked( fxMesa );
      }

      /* look for clipmasks, giveup if region obscured */
      if (fxMesa->glCtx->Color.DrawBuffer[0] == GL_FRONT) {
         if (!inClipRects_Region(fxMesa, scrX, scrY, width, height)) {
            UNLOCK_HARDWARE(fxMesa);
	    _swrast_DrawPixels( ctx, x, y, width, height, format, type, 
				unpack, pixels );
            return;
         }
      }

      info.size = sizeof(info);
      if (fxMesa->Glide.grLfbLock(GR_LFB_WRITE_ONLY,
                    fxMesa->DrawBuffer,
                    GR_LFBWRITEMODE_8888,
                    GR_ORIGIN_UPPER_LEFT, FXTRUE, &info))
      {
         const GLint dstStride = (fxMesa->glCtx->Color.DrawBuffer[0] == GL_FRONT)
            ? (fxMesa->screen_width * 4) : (info.strideInBytes);
         GLubyte *dst = (GLubyte *) info.lfbPtr
            + scrY * dstStride + scrX * 4;
         const GLint srcStride =
            _mesa_image_row_stride(unpack, width, format, type);
         const GLubyte *src = (GLubyte *) _mesa_image_address2d(unpack,
            pixels, width, height, format, type, 0, 0);
         const GLint widthInBytes = width * 4;

         if ((format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8) ||
             (format == GL_BGRA && type == GL_UNSIGNED_BYTE)) {
            GLint row;
            for (row = 0; row < height; row++) {
               MEMCPY(dst, src, widthInBytes);
               dst -= dstStride;
               src += srcStride;
            }
            result = GL_TRUE;
         }

         fxMesa->Glide.grLfbUnlock(GR_LFB_WRITE_ONLY, fxMesa->DrawBuffer);
      }
      UNLOCK_HARDWARE(fxMesa);
   }
}
