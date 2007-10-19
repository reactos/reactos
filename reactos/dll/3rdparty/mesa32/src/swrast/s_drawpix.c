/*
 * Mesa 3-D graphics library
 * Version:  6.4
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "bufferobj.h"
#include "context.h"
#include "convolve.h"
#include "image.h"
#include "macros.h"
#include "imports.h"
#include "pixel.h"

#include "s_context.h"
#include "s_drawpix.h"
#include "s_pixeltex.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_zoom.h"


/*
 * Try to do a fast and simple RGB(a) glDrawPixels.
 * Return:  GL_TRUE if success, GL_FALSE if slow path must be used instead
 */
static GLboolean
fast_draw_pixels(GLcontext *ctx, GLint x, GLint y,
                 GLsizei width, GLsizei height,
                 GLenum format, GLenum type,
                 const struct gl_pixelstore_attrib *unpack,
                 const GLvoid *pixels)
{
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0][0];
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct sw_span span;

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_RGBA);

   if (!ctx->Current.RasterPosValid) {
      return GL_TRUE;      /* no-op */
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT)
      return GL_FALSE;

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (ctx->Fog.Enabled)
      _swrast_span_default_fog(ctx, &span);
   if (ctx->Texture._EnabledCoordUnits)
      _swrast_span_default_texcoords(ctx, &span);

   if ((SWRAST_CONTEXT(ctx)->_RasterMask & ~CLIP_BIT) == 0
       && ctx->Texture._EnabledCoordUnits == 0
       && unpack->Alignment == 1
       && !unpack->SwapBytes
       && !unpack->LsbFirst) {

      GLint destX = x;
      GLint destY = y;
      GLint drawWidth = width;           /* actual width drawn */
      GLint drawHeight = height;         /* actual height drawn */
      GLint skipPixels = unpack->SkipPixels;
      GLint skipRows = unpack->SkipRows;
      GLint rowLength;
      GLint zoomY0 = 0;

      if (unpack->RowLength > 0)
         rowLength = unpack->RowLength;
      else
         rowLength = width;

      /* If we're not using pixel zoom then do all clipping calculations
       * now.  Otherwise, we'll let the _swrast_write_zoomed_*_span() functions
       * handle the clipping.
       */
      if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
         /* horizontal clipping */
         if (destX < ctx->DrawBuffer->_Xmin) {
            skipPixels += (ctx->DrawBuffer->_Xmin - destX);
            drawWidth  -= (ctx->DrawBuffer->_Xmin - destX);
            destX = ctx->DrawBuffer->_Xmin;
         }
         if (destX + drawWidth > ctx->DrawBuffer->_Xmax)
            drawWidth -= (destX + drawWidth - ctx->DrawBuffer->_Xmax);
         if (drawWidth <= 0)
            return GL_TRUE;

         /* vertical clipping */
         if (destY < ctx->DrawBuffer->_Ymin) {
            skipRows   += (ctx->DrawBuffer->_Ymin - destY);
            drawHeight -= (ctx->DrawBuffer->_Ymin - destY);
            destY = ctx->DrawBuffer->_Ymin;
         }
         if (destY + drawHeight > ctx->DrawBuffer->_Ymax)
            drawHeight -= (destY + drawHeight - ctx->DrawBuffer->_Ymax);
         if (drawHeight <= 0)
            return GL_TRUE;
      }
      else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
         /* upside-down image */
         /* horizontal clipping */
         if (destX < ctx->DrawBuffer->_Xmin) {
            skipPixels += (ctx->DrawBuffer->_Xmin - destX);
            drawWidth  -= (ctx->DrawBuffer->_Xmin - destX);
            destX = ctx->DrawBuffer->_Xmin;
         }
         if (destX + drawWidth > ctx->DrawBuffer->_Xmax)
            drawWidth -= (destX + drawWidth - ctx->DrawBuffer->_Xmax);
         if (drawWidth <= 0)
            return GL_TRUE;

         /* vertical clipping */
         if (destY > ctx->DrawBuffer->_Ymax) {
            skipRows   += (destY - ctx->DrawBuffer->_Ymax);
            drawHeight -= (destY - ctx->DrawBuffer->_Ymax);
            destY = ctx->DrawBuffer->_Ymax;
         }
         if (destY - drawHeight < ctx->DrawBuffer->_Ymin)
            drawHeight -= (ctx->DrawBuffer->_Ymin - (destY - drawHeight));
         if (drawHeight <= 0)
            return GL_TRUE;
      }
      else {
         if (drawWidth > MAX_WIDTH)
            return GL_FALSE; /* fall back to general case path */

         /* save Y value of first row */
         zoomY0 = IROUND(ctx->Current.RasterPos[1]);
      }


      /*
       * Ready to draw!
       * The window region at (destX, destY) of size (drawWidth, drawHeight)
       * will be written to.
       * We'll take pixel data from buffer pointed to by "pixels" but we'll
       * skip "skipRows" rows and skip "skipPixels" pixels/row.
       */

      if (format == GL_RGBA && type == CHAN_TYPE
          && ctx->_ImageTransferState==0) {
         if (ctx->Visual.rgbMode) {
            GLchan *src = (GLchan *) pixels
               + (skipRows * rowLength + skipPixels) * 4;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  rb->PutRow(ctx, rb, drawWidth, destX, destY, src, NULL);
                  src += rowLength * 4;
                  destY++;
               }
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  destY--;
                  rb->PutRow(ctx, rb, drawWidth, destX, destY, src, NULL);
                  src += rowLength * 4;
               }
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  span.x = destX;
                  span.y = destY;
                  span.end = drawWidth;
                  _swrast_write_zoomed_rgba_span(ctx, &span,
                                        (CONST GLchan (*)[4]) src, zoomY0, 0);
                  src += rowLength * 4;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format == GL_RGB && type == CHAN_TYPE
               && ctx->_ImageTransferState == 0) {
         if (ctx->Visual.rgbMode) {
            GLchan *src = (GLchan *) pixels
               + (skipRows * rowLength + skipPixels) * 3;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  rb->PutRowRGB(ctx, rb, drawWidth, destX, destY, src, NULL);
                  src += rowLength * 3;
                  destY++;
               }
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  destY--;
                  rb->PutRowRGB(ctx, rb, drawWidth, destX, destY, src, NULL);
                  src += rowLength * 3;
               }
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  span.x = destX;
                  span.y = destY;
                  span.end = drawWidth;
                  _swrast_write_zoomed_rgb_span(ctx, &span,
                                         (CONST GLchan (*)[3]) src, zoomY0, 0);
                  src += rowLength * 3;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format == GL_LUMINANCE && type == CHAN_TYPE
               && ctx->_ImageTransferState==0) {
         if (ctx->Visual.rgbMode) {
            GLchan *src = (GLchan *) pixels
               + (skipRows * rowLength + skipPixels);
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               ASSERT(drawWidth <= MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
		  for (i=0;i<drawWidth;i++) {
                     span.array->rgb[i][0] = src[i];
                     span.array->rgb[i][1] = src[i];
                     span.array->rgb[i][2] = src[i];
		  }
                  rb->PutRowRGB(ctx, rb, drawWidth, destX, destY,
                                span.array->rgb, NULL);
                  src += rowLength;
                  destY++;
               }
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               ASSERT(drawWidth <= MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  for (i=0;i<drawWidth;i++) {
                     span.array->rgb[i][0] = src[i];
                     span.array->rgb[i][1] = src[i];
                     span.array->rgb[i][2] = src[i];
                  }
                  destY--;
                  rb->PutRow(ctx, rb, drawWidth, destX, destY,
                             span.array->rgb, NULL);
                  src += rowLength;
               }
            }
            else {
               /* with zooming */
               GLint row;
               ASSERT(drawWidth <= MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
		  for (i=0;i<drawWidth;i++) {
                     span.array->rgb[i][0] = src[i];
                     span.array->rgb[i][1] = src[i];
                     span.array->rgb[i][2] = src[i];
		  }
                  span.x = destX;
                  span.y = destY;
                  span.end = drawWidth;
                  _swrast_write_zoomed_rgb_span(ctx, &span,
                             (CONST GLchan (*)[3]) span.array->rgb, zoomY0, 0);
                  src += rowLength;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format == GL_LUMINANCE_ALPHA && type == CHAN_TYPE
               && ctx->_ImageTransferState == 0) {
         if (ctx->Visual.rgbMode) {
            GLchan *src = (GLchan *) pixels
               + (skipRows * rowLength + skipPixels)*2;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               ASSERT(drawWidth <= MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  GLchan *ptr = src;
		  for (i=0;i<drawWidth;i++) {
                     span.array->rgba[i][0] = *ptr;
                     span.array->rgba[i][1] = *ptr;
                     span.array->rgba[i][2] = *ptr++;
                     span.array->rgba[i][3] = *ptr++;
		  }
                  rb->PutRow(ctx, rb, drawWidth, destX, destY,
                             span.array->rgba, NULL);
                  src += rowLength*2;
                  destY++;
               }
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               ASSERT(drawWidth <= MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLint i;
                  GLchan *ptr = src;
                  for (i=0;i<drawWidth;i++) {
                     span.array->rgba[i][0] = *ptr;
                     span.array->rgba[i][1] = *ptr;
                     span.array->rgba[i][2] = *ptr++;
                     span.array->rgba[i][3] = *ptr++;
                  }
                  destY--;
                  rb->PutRow(ctx, rb, drawWidth, destX, destY,
                             span.array->rgba, NULL);
                  src += rowLength*2;
               }
            }
            else {
               /* with zooming */
               GLint row;
               ASSERT(drawWidth <= MAX_WIDTH);
               for (row=0; row<drawHeight; row++) {
                  GLchan *ptr = src;
                  GLint i;
		  for (i=0;i<drawWidth;i++) {
                     span.array->rgba[i][0] = *ptr;
                     span.array->rgba[i][1] = *ptr;
                     span.array->rgba[i][2] = *ptr++;
                     span.array->rgba[i][3] = *ptr++;
		  }
                  span.x = destX;
                  span.y = destY;
                  span.end = drawWidth;
                  _swrast_write_zoomed_rgba_span(ctx, &span,
                            (CONST GLchan (*)[4]) span.array->rgba, zoomY0, 0);
                  src += rowLength*2;
                  destY++;
               }
            }
         }
         return GL_TRUE;
      }
      else if (format==GL_COLOR_INDEX && type==GL_UNSIGNED_BYTE) {
         GLubyte *src = (GLubyte *) pixels + skipRows * rowLength + skipPixels;
         if (ctx->Visual.rgbMode) {
            /* convert CI data to RGBA */
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  ASSERT(drawWidth <= MAX_WIDTH);
                  _mesa_map_ci8_to_rgba(ctx, drawWidth, src, span.array->rgba);
                  rb->PutRow(ctx, rb, drawWidth, destX, destY,
                             span.array->rgba, NULL);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            }
            else if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==-1.0F) {
               /* upside-down */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  ASSERT(drawWidth <= MAX_WIDTH);
                  _mesa_map_ci8_to_rgba(ctx, drawWidth, src, span.array->rgba);
                  destY--;
                  rb->PutRow(ctx, rb, drawWidth, destX, destY,
                             span.array->rgba, NULL);
                  src += rowLength;
               }
               return GL_TRUE;
            }
            else {
               /* with zooming */
               GLint row;
               for (row=0; row<drawHeight; row++) {
                  ASSERT(drawWidth <= MAX_WIDTH);
                  _mesa_map_ci8_to_rgba(ctx, drawWidth, src, span.array->rgba);
                  span.x = destX;
                  span.y = destY;
                  span.end = drawWidth;
                  _swrast_write_zoomed_rgba_span(ctx, &span,
                            (CONST GLchan (*)[4]) span.array->rgba, zoomY0, 0);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            }
         }
         else if (ctx->_ImageTransferState==0) {
            /* write CI data to CI frame buffer */
            GLint row;
            if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
               /* no zooming */
               for (row=0; row<drawHeight; row++) {
                  GLuint index32[MAX_WIDTH];
                  GLint col;
                  for (col = 0; col < drawWidth; col++)
                     index32[col] = src[col];
                  rb->PutRow(ctx, rb, drawWidth, destX, destY, index32, NULL);
                  src += rowLength;
                  destY++;
               }
               return GL_TRUE;
            }
            else {
               /* with zooming */
               return GL_FALSE;
            }
         }
      }
      else {
         /* can't handle this pixel format and/or data type here */
         return GL_FALSE;
      }
   }

   /* can't do a simple draw, have to use slow path */
   return GL_FALSE;
}



/*
 * Draw color index image.
 */
static void
draw_index_pixels( GLcontext *ctx, GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type,
                   const struct gl_pixelstore_attrib *unpack,
                   const GLvoid *pixels )
{
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;
   GLint row, skipPixels;
   struct sw_span span;

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_INDEX);

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (ctx->Fog.Enabled)
      _swrast_span_default_fog(ctx, &span);

   /*
    * General solution
    */
   skipPixels = 0;
   while (skipPixels < width) {
      const GLint spanX = x + (zoom ? 0 : skipPixels);
      GLint spanY = y;
      const GLint spanEnd = (width - skipPixels > MAX_WIDTH)
                          ? MAX_WIDTH : (width - skipPixels);
      ASSERT(spanEnd <= MAX_WIDTH);
      for (row = 0; row < height; row++, spanY++) {
         const GLvoid *source = _mesa_image_address2d(unpack, pixels,
                                                      width, height,
                                                      GL_COLOR_INDEX, type,
                                                      row, skipPixels);
         _mesa_unpack_index_span(ctx, spanEnd, GL_UNSIGNED_INT,
                                 span.array->index, type, source, unpack,
                                 ctx->_ImageTransferState);

         /* These may get changed during writing/clipping */
         span.x = spanX;
         span.y = spanY;
         span.end = spanEnd;

         if (zoom)
            _swrast_write_zoomed_index_span(ctx, &span, y, skipPixels);
         else
            _swrast_write_index_span(ctx, &span);
      }
      skipPixels += spanEnd;
   }
}



/*
 * Draw stencil image.
 */
static void
draw_stencil_pixels( GLcontext *ctx, GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum type,
                     const struct gl_pixelstore_attrib *unpack,
                     const GLvoid *pixels )
{
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;
   const GLint desty = y;
   GLint row, skipPixels;

   if (type != GL_BYTE &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_SHORT &&
       type != GL_UNSIGNED_SHORT &&
       type != GL_INT &&
       type != GL_UNSIGNED_INT &&
       type != GL_FLOAT &&
       type != GL_BITMAP) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glDrawPixels(stencil type)");
      return;
   }

   if (ctx->Visual.stencilBits == 0) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawPixels(no stencil buffer)");
      return;
   }

   /* if width > MAX_WIDTH, have to process image in chunks */
   skipPixels = 0;
   while (skipPixels < width) {
      const GLint spanX = x;
      GLint spanY = y;
      const GLint spanWidth = (width - skipPixels > MAX_WIDTH)
                            ? MAX_WIDTH : (width - skipPixels);

      for (row = 0; row < height; row++, spanY++) {
         GLstencil values[MAX_WIDTH];
         GLenum destType = (sizeof(GLstencil) == sizeof(GLubyte))
                         ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
         const GLvoid *source = _mesa_image_address2d(unpack, pixels,
                                                      width, height,
                                                      GL_COLOR_INDEX, type,
                                                      row, skipPixels);
         _mesa_unpack_index_span(ctx, spanWidth, destType, values,
                                 type, source, unpack,
                                 ctx->_ImageTransferState);
         if (ctx->_ImageTransferState & IMAGE_SHIFT_OFFSET_BIT) {
            _mesa_shift_and_offset_stencil(ctx, spanWidth, values);
         }
         if (ctx->Pixel.MapStencilFlag) {
            _mesa_map_stencil(ctx, spanWidth, values);
         }

         if (zoom) {
            _swrast_write_zoomed_stencil_span(ctx, (GLuint) spanWidth,
                                            spanX, spanY, values, desty, 0);
         }
         else {
            _swrast_write_stencil_span(ctx, spanWidth, spanX, spanY, values);
         }
      }
      skipPixels += spanWidth;
   }
}


/*
 * Draw depth image.
 */
static void
draw_depth_pixels( GLcontext *ctx, GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type,
                   const struct gl_pixelstore_attrib *unpack,
                   const GLvoid *pixels )
{
   const GLboolean bias_or_scale = ctx->Pixel.DepthBias!=0.0 || ctx->Pixel.DepthScale!=1.0;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0 || ctx->Pixel.ZoomY != 1.0;
   const GLint desty = y;
   struct sw_span span;

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_Z);

   if (type != GL_BYTE
       && type != GL_UNSIGNED_BYTE
       && type != GL_SHORT
       && type != GL_UNSIGNED_SHORT
       && type != GL_INT
       && type != GL_UNSIGNED_INT
       && type != GL_FLOAT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawPixels(type)");
      return;
   }

   _swrast_span_default_color(ctx, &span);

   if (ctx->Fog.Enabled)
      _swrast_span_default_fog(ctx, &span);
   if (ctx->Texture._EnabledCoordUnits)
      _swrast_span_default_texcoords(ctx, &span);

   if (type == GL_UNSIGNED_SHORT
       && ctx->Visual.depthBits == 16
       && !bias_or_scale
       && !zoom
       && ctx->Visual.rgbMode
       && width <= MAX_WIDTH) {
      /* Special case: directly write 16-bit depth values */
      GLint row, spanY = y;
      for (row = 0; row < height; row++, spanY++) {
         const GLushort *zSrc = (const GLushort *)
            _mesa_image_address2d(unpack, pixels, width, height,
                                  GL_DEPTH_COMPONENT, type, row, 0);
         GLint i;
         for (i = 0; i < width; i++)
            span.array->z[i] = zSrc[i];
         span.x = x;
         span.y = spanY;
         span.end = width;
         _swrast_write_rgba_span(ctx, &span);
      }
   }
   else if (type == GL_UNSIGNED_INT
            && sizeof(GLdepth) == 4
            && !bias_or_scale
            && !zoom
            && ctx->Visual.rgbMode
            && width <= MAX_WIDTH) {
      /* Special case: shift 32-bit values down to ctx->Visual.depthBits */
      const GLint shift = 32 - ctx->Visual.depthBits;
      GLint row, spanY = y;
      for (row = 0; row < height; row++, spanY++) {
         const GLuint *zSrc = (const GLuint *)
            _mesa_image_address2d(unpack, pixels, width, height,
                                  GL_DEPTH_COMPONENT, type, row, 0);
         if (shift == 0) {
            MEMCPY(span.array->z, zSrc, width * sizeof(GLdepth));
         }
         else {
            GLint col;
            for (col = 0; col < width; col++)
               span.array->z[col] = zSrc[col] >> shift;
         }
         span.x = x;
         span.y = spanY;
         span.end = width;
         _swrast_write_rgba_span(ctx, &span);
      }
   }
   else {
      /* General case */
      const GLfloat depthMax = ctx->DrawBuffer->_DepthMaxF;
      GLint row, skipPixels = 0;

      /* in case width > MAX_WIDTH do the copy in chunks */
      while (skipPixels < width) {
         const GLint spanX = x + (zoom ? 0 : skipPixels);
         GLint spanY = y;
         const GLint spanEnd = (width - skipPixels > MAX_WIDTH)
                             ? MAX_WIDTH : (width - skipPixels);
         ASSERT(span.end <= MAX_WIDTH);
         for (row = 0; row < height; row++, spanY++) {
            GLfloat floatSpan[MAX_WIDTH];
            const GLvoid *zSrc = _mesa_image_address2d(unpack,
                                                      pixels, width, height,
                                                      GL_DEPTH_COMPONENT, type,
                                                      row, skipPixels);

            /* Set these for each row since the _swrast_write_* function may
             * change them while clipping.
             */
            span.x = spanX;
            span.y = spanY;
            span.end = spanEnd;

            _mesa_unpack_depth_span(ctx, span.end, floatSpan, type,
                                    zSrc, unpack);
            /* clamp depth values to [0,1] and convert from floats to ints */
            {
               GLuint i;
               for (i = 0; i < span.end; i++) {
                  span.array->z[i] = (GLdepth) (floatSpan[i] * depthMax);
               }
            }
            if (zoom) {
               _swrast_write_zoomed_depth_span(ctx, &span, desty, skipPixels);
            }
            else if (ctx->Visual.rgbMode) {
               _swrast_write_rgba_span(ctx, &span);
            }
            else {
               _swrast_write_index_span(ctx, &span);
            }
         }
         skipPixels += spanEnd;
      }
   }
}



/*
 * Draw RGBA image.
 */
static void
draw_rgba_pixels( GLcontext *ctx, GLint x, GLint y,
                  GLsizei width, GLsizei height,
                  GLenum format, GLenum type,
                  const struct gl_pixelstore_attrib *unpack,
                  const GLvoid *pixels )
{
   struct gl_renderbuffer *rb = NULL; /* only used for quickDraw path */
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;
   const GLint desty = y;
   GLboolean quickDraw;
   GLfloat *convImage = NULL;
   GLuint transferOps = ctx->_ImageTransferState;
   struct sw_span span;

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_RGBA);

   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawPixels(format or type)");
      return;
   }

   /* Try an optimized glDrawPixels first */
   if (fast_draw_pixels(ctx, x, y, width, height, format, type, unpack, pixels))
      return;

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (ctx->Fog.Enabled)
      _swrast_span_default_fog(ctx, &span);
   if (ctx->Texture._EnabledCoordUnits)
      _swrast_span_default_texcoords(ctx, &span);

   if (SWRAST_CONTEXT(ctx)->_RasterMask == 0 && !zoom && x >= 0 && y >= 0
       && x + width <= (GLint) ctx->DrawBuffer->Width
       && y + height <= (GLint) ctx->DrawBuffer->Height
       && ctx->DrawBuffer->_NumColorDrawBuffers[0] == 1) {
      quickDraw = GL_TRUE;
      rb = ctx->DrawBuffer->_ColorDrawBuffers[0][0];
   }
   else {
      quickDraw = GL_FALSE;
      rb = NULL;
   }

   if (ctx->Pixel.Convolution2DEnabled || ctx->Pixel.Separable2DEnabled) {
      /* Convolution has to be handled specially.  We'll create an
       * intermediate image, applying all pixel transfer operations
       * up to convolution.  Then we'll convolve the image.  Then
       * we'll proceed with the rest of the transfer operations and
       * rasterize the image.
       */
      GLint row;
      GLfloat *dest, *tmpImage;

      tmpImage = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
      if (!tmpImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDrawPixels");
         return;
      }
      convImage = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
      if (!convImage) {
         _mesa_free(tmpImage);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDrawPixels");
         return;
      }

      /* Unpack the image and apply transfer ops up to convolution */
      dest = tmpImage;
      for (row = 0; row < height; row++) {
         const GLvoid *source = _mesa_image_address2d(unpack,
                                  pixels, width, height, format, type, row, 0);
         _mesa_unpack_color_span_float(ctx, width, GL_RGBA, (GLfloat *) dest,
                                     format, type, source, unpack,
                                     transferOps & IMAGE_PRE_CONVOLUTION_BITS);
         dest += width * 4;
      }

      /* do convolution */
      if (ctx->Pixel.Convolution2DEnabled) {
         _mesa_convolve_2d_image(ctx, &width, &height, tmpImage, convImage);
      }
      else {
         ASSERT(ctx->Pixel.Separable2DEnabled);
         _mesa_convolve_sep_image(ctx, &width, &height, tmpImage, convImage);
      }
      _mesa_free(tmpImage);

      /* continue transfer ops and draw the convolved image */
      unpack = &ctx->DefaultPacking;
      pixels = convImage;
      format = GL_RGBA;
      type = GL_FLOAT;
      transferOps &= IMAGE_POST_CONVOLUTION_BITS;
   }

   /*
    * General solution
    */
   {
      const GLuint interpMask = span.interpMask;
      const GLuint arrayMask = span.arrayMask;
      GLint row, skipPixels = 0;

      /* if the span is wider than MAX_WIDTH we have to do it in chunks */
      while (skipPixels < width) {
         const GLint spanX = x + (zoom ? 0 : skipPixels);
         GLint spanY = y;
         const GLint spanEnd = (width - skipPixels > MAX_WIDTH)
                             ? MAX_WIDTH : (width - skipPixels);
         ASSERT(span.end <= MAX_WIDTH);

         for (row = 0; row < height; row++, spanY++) {
            const GLvoid *source = _mesa_image_address2d(unpack,
                     pixels, width, height, format, type, row, skipPixels);

            /* Set these for each row since the _swrast_write_* function may
             * change them while clipping.
             */
            span.x = spanX;
            span.y = spanY;
            span.end = spanEnd;
            span.arrayMask = arrayMask;
            span.interpMask = interpMask;

            _mesa_unpack_color_span_chan(ctx, span.end, GL_RGBA,
                                         (GLchan *) span.array->rgba,
                                         format, type, source, unpack,
                                         transferOps);

            if ((ctx->Pixel.MinMaxEnabled && ctx->MinMax.Sink) ||
                (ctx->Pixel.HistogramEnabled && ctx->Histogram.Sink))
               continue;

            if (ctx->Pixel.PixelTextureEnabled && ctx->Texture._EnabledUnits) {
               _swrast_pixel_texture(ctx, &span);
            }

            /* draw the span */
            if (quickDraw) {
               rb->PutRow(ctx, rb, span.end, span.x, span.y,
                          span.array->rgba, NULL);
            }
            else if (zoom) {
               _swrast_write_zoomed_rgba_span(ctx, &span,
                    (CONST GLchan (*)[4]) span.array->rgba, desty, skipPixels);
            }
            else {
               _swrast_write_rgba_span(ctx, &span);
            }
         }

         skipPixels += spanEnd;
      }
   }

   if (convImage) {
      _mesa_free(convImage);
   }
}



/*
 * Execute glDrawPixels
 */
void
_swrast_DrawPixels( GLcontext *ctx,
		    GLint x, GLint y,
		    GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    const GLvoid *pixels )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (unpack->BufferObj->Name) {
      /* unpack from PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(2, unpack, width, height, 1,
                                     format, type, pixels)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glDrawPixels(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              unpack->BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawPixels(PBO is mapped)");
         return;
      }
      pixels = ADD_POINTERS(buf, pixels);
   }

   RENDER_START(swrast,ctx);

   switch (format) {
   case GL_STENCIL_INDEX:
      draw_stencil_pixels( ctx, x, y, width, height, type, unpack, pixels );
      break;
   case GL_DEPTH_COMPONENT:
      draw_depth_pixels( ctx, x, y, width, height, type, unpack, pixels );
      break;
   case GL_COLOR_INDEX:
      if (ctx->Visual.rgbMode)
	 draw_rgba_pixels(ctx, x,y, width, height, format, type, unpack, pixels);
      else
	 draw_index_pixels(ctx, x, y, width, height, type, unpack, pixels);
      break;
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_LUMINANCE_ALPHA:
   case GL_RGB:
   case GL_BGR:
   case GL_RGBA:
   case GL_BGRA:
   case GL_ABGR_EXT:
      draw_rgba_pixels(ctx, x, y, width, height, format, type, unpack, pixels);
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glDrawPixels(format)" );
      /* don't return yet, clean-up */
   }

   RENDER_FINISH(swrast,ctx);

   if (unpack->BufferObj->Name) {
      /* done with PBO so unmap it now */
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              unpack->BufferObj);
   }
}



#if 0  /* experimental */
/*
 * Execute glDrawDepthPixelsMESA().
 */
void
_swrast_DrawDepthPixelsMESA( GLcontext *ctx,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height,
                             GLenum colorFormat, GLenum colorType,
                             const GLvoid *colors,
                             GLenum depthType, const GLvoid *depths,
                             const struct gl_pixelstore_attrib *unpack )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   RENDER_START(swrast,ctx);

   switch (colorFormat) {
   case GL_COLOR_INDEX:
      if (ctx->Visual.rgbMode)
	 draw_rgba_pixels(ctx, x,y, width, height, colorFormat, colorType, unpack, colors);
      else
	 draw_index_pixels(ctx, x, y, width, height, colorType, unpack, colors);
      break;
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_LUMINANCE_ALPHA:
   case GL_RGB:
   case GL_BGR:
   case GL_RGBA:
   case GL_BGRA:
   case GL_ABGR_EXT:
      draw_rgba_pixels(ctx, x, y, width, height, colorFormat, colorType, unpack, colors);
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM,
                   "glDrawDepthPixelsMESA(colorFormat)" );
   }

   RENDER_FINISH(swrast,ctx);
}
#endif
