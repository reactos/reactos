/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/convolve.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/imports.h"
#include "main/pixel.h"
#include "main/state.h"

#include "s_context.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_zoom.h"



/**
 * Try to do a fast and simple RGB(a) glDrawPixels.
 * Return:  GL_TRUE if success, GL_FALSE if slow path must be used instead
 */
static GLboolean
fast_draw_rgba_pixels(GLcontext *ctx, GLint x, GLint y,
                      GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const struct gl_pixelstore_attrib *userUnpack,
                      const GLvoid *pixels)
{
   const GLint imgX = x, imgY = y;
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0];
   GLenum rbType;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   SWspan span;
   GLboolean simpleZoom;
   GLint yStep;  /* +1 or -1 */
   struct gl_pixelstore_attrib unpack;
   GLint destX, destY, drawWidth, drawHeight; /* post clipping */

   if (!rb)
      return GL_TRUE; /* no-op */

   rbType = rb->DataType;

   if ((swrast->_RasterMask & ~CLIP_BIT) ||
       ctx->Texture._EnabledCoordUnits ||
       userUnpack->SwapBytes ||
       ctx->_ImageTransferState) {
      /* can't handle any of those conditions */
      return GL_FALSE;
   }

   INIT_SPAN(span, GL_BITMAP);
   span.arrayMask = SPAN_RGBA;
   span.arrayAttribs = FRAG_BIT_COL0;
   _swrast_span_default_attribs(ctx, &span);

   /* copy input params since clipping may change them */
   unpack = *userUnpack;
   destX = x;
   destY = y;
   drawWidth = width;
   drawHeight = height;

   /* check for simple zooming and clipping */
   if (ctx->Pixel.ZoomX == 1.0F &&
       (ctx->Pixel.ZoomY == 1.0F || ctx->Pixel.ZoomY == -1.0F)) {
      if (!_mesa_clip_drawpixels(ctx, &destX, &destY,
                                 &drawWidth, &drawHeight, &unpack)) {
         /* image was completely clipped: no-op, all done */
         return GL_TRUE;
      }
      simpleZoom = GL_TRUE;
      yStep = (GLint) ctx->Pixel.ZoomY;
      ASSERT(yStep == 1 || yStep == -1);
   }
   else {
      /* non-simple zooming */
      simpleZoom = GL_FALSE;
      yStep = 1;
      if (unpack.RowLength == 0)
         unpack.RowLength = width;
   }

   /*
    * Ready to draw!
    */

   if (format == GL_RGBA && type == rbType) {
      const GLubyte *src
         = (const GLubyte *) _mesa_image_address2d(&unpack, pixels, width,
                                                   height, format, type, 0, 0);
      const GLint srcStride = _mesa_image_row_stride(&unpack, width,
                                                     format, type);
      if (simpleZoom) {
         GLint row;
         for (row = 0; row < drawHeight; row++) {
            rb->PutRow(ctx, rb, drawWidth, destX, destY, src, NULL);
            src += srcStride;
            destY += yStep;
         }
      }
      else {
         /* with zooming */
         GLint row;
         for (row = 0; row < drawHeight; row++) {
            span.x = destX;
            span.y = destY + row;
            span.end = drawWidth;
            span.array->ChanType = rbType;
            _swrast_write_zoomed_rgba_span(ctx, imgX, imgY, &span, src);
            src += srcStride;
         }
         span.array->ChanType = CHAN_TYPE;
      }
      return GL_TRUE;
   }

   if (format == GL_RGB && type == rbType) {
      const GLubyte *src
         = (const GLubyte *) _mesa_image_address2d(&unpack, pixels, width,
                                                   height, format, type, 0, 0);
      const GLint srcStride = _mesa_image_row_stride(&unpack, width,
                                                     format, type);
      if (simpleZoom) {
         GLint row;
         for (row = 0; row < drawHeight; row++) {
            rb->PutRowRGB(ctx, rb, drawWidth, destX, destY, src, NULL);
            src += srcStride;
            destY += yStep;
         }
      }
      else {
         /* with zooming */
         GLint row;
         for (row = 0; row < drawHeight; row++) {
            span.x = destX;
            span.y = destY;
            span.end = drawWidth;
            span.array->ChanType = rbType;
            _swrast_write_zoomed_rgb_span(ctx, imgX, imgY, &span, src);
            src += srcStride;
            destY++;
         }
         span.array->ChanType = CHAN_TYPE;
      }
      return GL_TRUE;
   }

   /* Remaining cases haven't been tested with alignment != 1 */
   if (userUnpack->Alignment != 1)
      return GL_FALSE;

   if (format == GL_LUMINANCE && type == CHAN_TYPE && rbType == CHAN_TYPE) {
      const GLchan *src = (const GLchan *) pixels
         + (unpack.SkipRows * unpack.RowLength + unpack.SkipPixels);
      if (simpleZoom) {
         /* no zooming */
         GLint row;
         ASSERT(drawWidth <= MAX_WIDTH);
         for (row = 0; row < drawHeight; row++) {
            GLchan rgb[MAX_WIDTH][3];
            GLint i;
            for (i = 0;i<drawWidth;i++) {
               rgb[i][0] = src[i];
               rgb[i][1] = src[i];
               rgb[i][2] = src[i];
            }
            rb->PutRowRGB(ctx, rb, drawWidth, destX, destY, rgb, NULL);
            src += unpack.RowLength;
            destY += yStep;
         }
      }
      else {
         /* with zooming */
         GLint row;
         ASSERT(drawWidth <= MAX_WIDTH);
         for (row = 0; row < drawHeight; row++) {
            GLchan rgb[MAX_WIDTH][3];
            GLint i;
            for (i = 0;i<drawWidth;i++) {
               rgb[i][0] = src[i];
               rgb[i][1] = src[i];
               rgb[i][2] = src[i];
            }
            span.x = destX;
            span.y = destY;
            span.end = drawWidth;
            _swrast_write_zoomed_rgb_span(ctx, imgX, imgY, &span, rgb);
            src += unpack.RowLength;
            destY++;
         }
      }
      return GL_TRUE;
   }

   if (format == GL_LUMINANCE_ALPHA && type == CHAN_TYPE && rbType == CHAN_TYPE) {
      const GLchan *src = (const GLchan *) pixels
         + (unpack.SkipRows * unpack.RowLength + unpack.SkipPixels)*2;
      if (simpleZoom) {
         GLint row;
         ASSERT(drawWidth <= MAX_WIDTH);
         for (row = 0; row < drawHeight; row++) {
            GLint i;
            const GLchan *ptr = src;
            for (i = 0;i<drawWidth;i++) {
               span.array->rgba[i][0] = *ptr;
               span.array->rgba[i][1] = *ptr;
               span.array->rgba[i][2] = *ptr++;
               span.array->rgba[i][3] = *ptr++;
            }
            rb->PutRow(ctx, rb, drawWidth, destX, destY,
                       span.array->rgba, NULL);
            src += unpack.RowLength*2;
            destY += yStep;
         }
      }
      else {
         /* with zooming */
         GLint row;
         ASSERT(drawWidth <= MAX_WIDTH);
         for (row = 0; row < drawHeight; row++) {
            const GLchan *ptr = src;
            GLint i;
            for (i = 0;i<drawWidth;i++) {
               span.array->rgba[i][0] = *ptr;
               span.array->rgba[i][1] = *ptr;
               span.array->rgba[i][2] = *ptr++;
               span.array->rgba[i][3] = *ptr++;
            }
            span.x = destX;
            span.y = destY;
            span.end = drawWidth;
            _swrast_write_zoomed_rgba_span(ctx, imgX, imgY, &span,
                                           span.array->rgba);
            src += unpack.RowLength*2;
            destY++;
         }
      }
      return GL_TRUE;
   }

   if (format == GL_COLOR_INDEX && type == GL_UNSIGNED_BYTE) {
      const GLubyte *src = (const GLubyte *) pixels
         + unpack.SkipRows * unpack.RowLength + unpack.SkipPixels;
      if (ctx->Visual.rgbMode && rbType == GL_UNSIGNED_BYTE) {
         /* convert ubyte/CI data to ubyte/RGBA */
         if (simpleZoom) {
            GLint row;
            for (row = 0; row < drawHeight; row++) {
               ASSERT(drawWidth <= MAX_WIDTH);
               _mesa_map_ci8_to_rgba8(ctx, drawWidth, src,
                                      span.array->rgba8);
               rb->PutRow(ctx, rb, drawWidth, destX, destY,
                          span.array->rgba8, NULL);
               src += unpack.RowLength;
               destY += yStep;
            }
         }
         else {
            /* ubyte/CI to ubyte/RGBA with zooming */
            GLint row;
            for (row = 0; row < drawHeight; row++) {
               ASSERT(drawWidth <= MAX_WIDTH);
               _mesa_map_ci8_to_rgba8(ctx, drawWidth, src,
                                      span.array->rgba8);
               span.x = destX;
               span.y = destY;
               span.end = drawWidth;
               _swrast_write_zoomed_rgba_span(ctx, imgX, imgY, &span,
                                              span.array->rgba8);
               src += unpack.RowLength;
               destY++;
            }
         }
         return GL_TRUE;
      }
      else if (!ctx->Visual.rgbMode && rbType == GL_UNSIGNED_INT) {
         /* write CI data to CI frame buffer */
         GLint row;
         if (simpleZoom) {
            for (row = 0; row < drawHeight; row++) {
               GLuint index32[MAX_WIDTH];
               GLint col;
               for (col = 0; col < drawWidth; col++)
                  index32[col] = src[col];
               rb->PutRow(ctx, rb, drawWidth, destX, destY, index32, NULL);
               src += unpack.RowLength;
               destY += yStep;
            }
            return GL_TRUE;
         }
      }
   }

   /* can't handle this pixel format and/or data type */
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
   const GLint imgX = x, imgY = y;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;
   GLint row, skipPixels;
   SWspan span;

   INIT_SPAN(span, GL_BITMAP);
   span.arrayMask = SPAN_INDEX;
   _swrast_span_default_attribs(ctx, &span);

   /*
    * General solution
    */
   skipPixels = 0;
   while (skipPixels < width) {
      const GLint spanWidth = MIN2(width - skipPixels, MAX_WIDTH);
      ASSERT(spanWidth <= MAX_WIDTH);
      for (row = 0; row < height; row++) {
         const GLvoid *source = _mesa_image_address2d(unpack, pixels,
                                                      width, height,
                                                      GL_COLOR_INDEX, type,
                                                      row, skipPixels);
         _mesa_unpack_index_span(ctx, spanWidth, GL_UNSIGNED_INT,
                                 span.array->index, type, source, unpack,
                                 ctx->_ImageTransferState);

         /* These may get changed during writing/clipping */
         span.x = x + skipPixels;
         span.y = y + row;
         span.end = spanWidth;
         
         if (zoom)
            _swrast_write_zoomed_index_span(ctx, imgX, imgY, &span);
         else
            _swrast_write_index_span(ctx, &span);
      }
      skipPixels += spanWidth;
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
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0 || ctx->Pixel.ZoomY != 1.0;
   GLint skipPixels;

   /* if width > MAX_WIDTH, have to process image in chunks */
   skipPixels = 0;
   while (skipPixels < width) {
      const GLint spanX = x + skipPixels;
      const GLint spanWidth = MIN2(width - skipPixels, MAX_WIDTH);
      GLint row;
      for (row = 0; row < height; row++) {
         const GLint spanY = y + row;
         GLstencil values[MAX_WIDTH];
         GLenum destType = (sizeof(GLstencil) == sizeof(GLubyte))
                         ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
         const GLvoid *source = _mesa_image_address2d(unpack, pixels,
                                                      width, height,
                                                      GL_COLOR_INDEX, type,
                                                      row, skipPixels);
         _mesa_unpack_stencil_span(ctx, spanWidth, destType, values,
                                   type, source, unpack,
                                   ctx->_ImageTransferState);
         if (zoom) {
            _swrast_write_zoomed_stencil_span(ctx, x, y, spanWidth,
                                              spanX, spanY, values);
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
   const GLboolean scaleOrBias
      = ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0 || ctx->Pixel.ZoomY != 1.0;
   SWspan span;

   INIT_SPAN(span, GL_BITMAP);
   span.arrayMask = SPAN_Z;
   _swrast_span_default_attribs(ctx, &span);

   if (type == GL_UNSIGNED_SHORT
       && ctx->DrawBuffer->Visual.depthBits == 16
       && !scaleOrBias
       && !zoom
       && ctx->Visual.rgbMode
       && width <= MAX_WIDTH
       && !unpack->SwapBytes) {
      /* Special case: directly write 16-bit depth values */
      GLint row;
      for (row = 0; row < height; row++) {
         const GLushort *zSrc = (const GLushort *)
            _mesa_image_address2d(unpack, pixels, width, height,
                                  GL_DEPTH_COMPONENT, type, row, 0);
         GLint i;
         for (i = 0; i < width; i++)
            span.array->z[i] = zSrc[i];
         span.x = x;
         span.y = y + row;
         span.end = width;
         _swrast_write_rgba_span(ctx, &span);
      }
   }
   else if (type == GL_UNSIGNED_INT
            && !scaleOrBias
            && !zoom
            && ctx->Visual.rgbMode
            && width <= MAX_WIDTH
            && !unpack->SwapBytes) {
      /* Special case: shift 32-bit values down to Visual.depthBits */
      const GLint shift = 32 - ctx->DrawBuffer->Visual.depthBits;
      GLint row;
      for (row = 0; row < height; row++) {
         const GLuint *zSrc = (const GLuint *)
            _mesa_image_address2d(unpack, pixels, width, height,
                                  GL_DEPTH_COMPONENT, type, row, 0);
         if (shift == 0) {
            _mesa_memcpy(span.array->z, zSrc, width * sizeof(GLuint));
         }
         else {
            GLint col;
            for (col = 0; col < width; col++)
               span.array->z[col] = zSrc[col] >> shift;
         }
         span.x = x;
         span.y = y + row;
         span.end = width;
         _swrast_write_rgba_span(ctx, &span);
      }
   }
   else {
      /* General case */
      const GLuint depthMax = ctx->DrawBuffer->_DepthMax;
      GLint skipPixels = 0;

      /* in case width > MAX_WIDTH do the copy in chunks */
      while (skipPixels < width) {
         const GLint spanWidth = MIN2(width - skipPixels, MAX_WIDTH);
         GLint row;
         ASSERT(span.end <= MAX_WIDTH);
         for (row = 0; row < height; row++) {
            const GLvoid *zSrc = _mesa_image_address2d(unpack,
                                                      pixels, width, height,
                                                      GL_DEPTH_COMPONENT, type,
                                                      row, skipPixels);

            /* Set these for each row since the _swrast_write_* function may
             * change them while clipping.
             */
            span.x = x + skipPixels;
            span.y = y + row;
            span.end = spanWidth;

            _mesa_unpack_depth_span(ctx, spanWidth,
                                    GL_UNSIGNED_INT, span.array->z, depthMax,
                                    type, zSrc, unpack);
            if (zoom) {
               _swrast_write_zoomed_depth_span(ctx, x, y, &span);
            }
            else if (ctx->Visual.rgbMode) {
               _swrast_write_rgba_span(ctx, &span);
            }
            else {
               _swrast_write_index_span(ctx, &span);
            }
         }
         skipPixels += spanWidth;
      }
   }
}



/**
 * Draw RGBA image.
 */
static void
draw_rgba_pixels( GLcontext *ctx, GLint x, GLint y,
                  GLsizei width, GLsizei height,
                  GLenum format, GLenum type,
                  const struct gl_pixelstore_attrib *unpack,
                  const GLvoid *pixels )
{
   const GLint imgX = x, imgY = y;
   const GLboolean zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;
   GLfloat *convImage = NULL;
   GLbitfield transferOps = ctx->_ImageTransferState;
   SWspan span;

   /* Try an optimized glDrawPixels first */
   if (fast_draw_rgba_pixels(ctx, x, y, width, height, format, type,
                             unpack, pixels)) {
      return;
   }

   INIT_SPAN(span, GL_BITMAP);
   _swrast_span_default_attribs(ctx, &span);
   span.arrayMask = SPAN_RGBA;
   span.arrayAttribs = FRAG_BIT_COL0; /* we're fill in COL0 attrib values */

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
   else if (ctx->Pixel.Convolution1DEnabled) {
      /* we only want to apply 1D convolution to glTexImage1D */
      transferOps &= ~(IMAGE_CONVOLUTION_BIT |
                       IMAGE_POST_CONVOLUTION_SCALE_BIAS);
   }

   if (ctx->DrawBuffer->_NumColorDrawBuffers > 0 &&
       ctx->DrawBuffer->_ColorDrawBuffers[0]->DataType != GL_FLOAT &&
       ctx->Color.ClampFragmentColor != GL_FALSE) {
      /* need to clamp colors before applying fragment ops */
      transferOps |= IMAGE_CLAMP_BIT;
   }

   /*
    * General solution
    */
   {
      const GLboolean sink = (ctx->Pixel.MinMaxEnabled && ctx->MinMax.Sink)
         || (ctx->Pixel.HistogramEnabled && ctx->Histogram.Sink);
      const GLbitfield interpMask = span.interpMask;
      const GLbitfield arrayMask = span.arrayMask;
      const GLint srcStride
         = _mesa_image_row_stride(unpack, width, format, type);
      GLint skipPixels = 0;
      /* use span array for temp color storage */
      GLfloat *rgba = (GLfloat *) span.array->attribs[FRAG_ATTRIB_COL0];

      /* if the span is wider than MAX_WIDTH we have to do it in chunks */
      while (skipPixels < width) {
         const GLint spanWidth = MIN2(width - skipPixels, MAX_WIDTH);
         const GLubyte *source
            = (const GLubyte *) _mesa_image_address2d(unpack, pixels,
                                                      width, height, format,
                                                      type, 0, skipPixels);
         GLint row;

         for (row = 0; row < height; row++) {
            /* get image row as float/RGBA */
            _mesa_unpack_color_span_float(ctx, spanWidth, GL_RGBA, rgba,
                                     format, type, source, unpack,
                                     transferOps);
            /* draw the span */
            if (!sink) {
               /* Set these for each row since the _swrast_write_* functions
                * may change them while clipping/rendering.
                */
               span.array->ChanType = GL_FLOAT;
               span.x = x + skipPixels;
               span.y = y + row;
               span.end = spanWidth;
               span.arrayMask = arrayMask;
               span.interpMask = interpMask;
               if (zoom) {
                  _swrast_write_zoomed_rgba_span(ctx, imgX, imgY, &span, rgba);
               }
               else {
                  _swrast_write_rgba_span(ctx, &span);
               }
            }

            source += srcStride;
         } /* for row */

         skipPixels += spanWidth;
      } /* while skipPixels < width */

      /* XXX this is ugly/temporary, to undo above change */
      span.array->ChanType = CHAN_TYPE;
   }

   if (convImage) {
      _mesa_free(convImage);
   }
}


/**
 * This is a bit different from drawing GL_DEPTH_COMPONENT pixels.
 * The only per-pixel operations that apply are depth scale/bias,
 * stencil offset/shift, GL_DEPTH_WRITEMASK and GL_STENCIL_WRITEMASK,
 * and pixel zoom.
 * Also, only the depth buffer and stencil buffers are touched, not the
 * color buffer(s).
 */
static void
draw_depth_stencil_pixels(GLcontext *ctx, GLint x, GLint y,
                          GLsizei width, GLsizei height, GLenum type,
                          const struct gl_pixelstore_attrib *unpack,
                          const GLvoid *pixels)
{
   const GLint imgX = x, imgY = y;
   const GLboolean scaleOrBias
      = ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0;
   const GLuint depthMax = ctx->DrawBuffer->_DepthMax;
   const GLuint stencilMask = ctx->Stencil.WriteMask[0];
   const GLuint stencilType = (STENCIL_BITS == 8) ? 
      GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0 || ctx->Pixel.ZoomY != 1.0;
   struct gl_renderbuffer *depthRb, *stencilRb;
   struct gl_pixelstore_attrib clippedUnpack = *unpack;

   if (!zoom) {
      if (!_mesa_clip_drawpixels(ctx, &x, &y, &width, &height,
                                 &clippedUnpack)) {
         /* totally clipped */
         return;
      }
   }
   
   depthRb = ctx->ReadBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   stencilRb = ctx->ReadBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
   ASSERT(depthRb);
   ASSERT(stencilRb);

   if (depthRb->_BaseFormat == GL_DEPTH_STENCIL_EXT &&
       stencilRb->_BaseFormat == GL_DEPTH_STENCIL_EXT &&
       depthRb == stencilRb &&
       !scaleOrBias &&
       !zoom &&
       ctx->Depth.Mask &&
       (stencilMask & 0xff) == 0xff) {
      /* This is the ideal case.
       * Drawing GL_DEPTH_STENCIL pixels into a combined depth/stencil buffer.
       * Plus, no pixel transfer ops, zooming, or masking needed.
       */
      GLint i;
      for (i = 0; i < height; i++) {
         const GLuint *src = (const GLuint *) 
            _mesa_image_address2d(&clippedUnpack, pixels, width, height,
                                  GL_DEPTH_STENCIL_EXT, type, i, 0);
         depthRb->PutRow(ctx, depthRb, width, x, y + i, src, NULL);
      }
   }
   else {
      /* sub-optimal cases:
       * Separate depth/stencil buffers, or pixel transfer ops required.
       */
      /* XXX need to handle very wide images (skippixels) */
      GLint i;

      depthRb = ctx->DrawBuffer->_DepthBuffer;
      stencilRb = ctx->DrawBuffer->_StencilBuffer;

      for (i = 0; i < height; i++) {
         const GLuint *depthStencilSrc = (const GLuint *)
            _mesa_image_address2d(&clippedUnpack, pixels, width, height,
                                  GL_DEPTH_STENCIL_EXT, type, i, 0);

         if (ctx->Depth.Mask) {
            if (!scaleOrBias && ctx->DrawBuffer->Visual.depthBits == 24) {
               /* fast path 24-bit zbuffer */
               GLuint zValues[MAX_WIDTH];
               GLint j;
               ASSERT(depthRb->DataType == GL_UNSIGNED_INT);
               for (j = 0; j < width; j++) {
                  zValues[j] = depthStencilSrc[j] >> 8;
               }
               if (zoom)
                  _swrast_write_zoomed_z_span(ctx, imgX, imgY, width,
                                              x, y + i, zValues);
               else
                  depthRb->PutRow(ctx, depthRb, width, x, y + i, zValues,NULL);
            }
            else if (!scaleOrBias && ctx->DrawBuffer->Visual.depthBits == 16) {
               /* fast path 16-bit zbuffer */
               GLushort zValues[MAX_WIDTH];
               GLint j;
               ASSERT(depthRb->DataType == GL_UNSIGNED_SHORT);
               for (j = 0; j < width; j++) {
                  zValues[j] = depthStencilSrc[j] >> 16;
               }
               if (zoom)
                  _swrast_write_zoomed_z_span(ctx, imgX, imgY, width,
                                              x, y + i, zValues);
               else
                  depthRb->PutRow(ctx, depthRb, width, x, y + i, zValues,NULL);
            }
            else {
               /* general case */
               GLuint zValues[MAX_WIDTH];  /* 16 or 32-bit Z value storage */
               _mesa_unpack_depth_span(ctx, width,
                                       depthRb->DataType, zValues, depthMax,
                                       type, depthStencilSrc, &clippedUnpack);
               if (zoom) {
                  _swrast_write_zoomed_z_span(ctx, imgX, imgY, width, x,
                                              y + i, zValues);
               }
               else {
                  depthRb->PutRow(ctx, depthRb, width, x, y + i, zValues,NULL);
               }
            }
         }

         if (stencilMask != 0x0) {
            GLstencil stencilValues[MAX_WIDTH];
            /* get stencil values, with shift/offset/mapping */
            _mesa_unpack_stencil_span(ctx, width, stencilType, stencilValues,
                                      type, depthStencilSrc, &clippedUnpack,
                                      ctx->_ImageTransferState);
            if (zoom)
               _swrast_write_zoomed_stencil_span(ctx, imgX, imgY, width,
                                                  x, y + i, stencilValues);
            else
               _swrast_write_stencil_span(ctx, width, x, y + i, stencilValues);
         }
      }
   }
}


/**
 * Execute software-based glDrawPixels.
 * By time we get here, all error checking will have been done.
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

   RENDER_START(swrast,ctx);

   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

    pixels = _mesa_map_drawpix_pbo(ctx, unpack, pixels);
    if (!pixels) {
       RENDER_FINISH(swrast,ctx);
       return;
    }

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
   case GL_DEPTH_STENCIL_EXT:
      draw_depth_stencil_pixels(ctx, x, y, width, height,
                                type, unpack, pixels);
      break;
   default:
      _mesa_problem(ctx, "unexpected format in _swrast_DrawPixels");
      /* don't return yet, clean-up */
   }

   RENDER_FINISH(swrast,ctx);

   _mesa_unmap_drapix_pbo(ctx, unpack);
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
	 draw_rgba_pixels(ctx, x,y, width, height, colorFormat, colorType,
                          unpack, colors);
      else
	 draw_index_pixels(ctx, x, y, width, height, colorType,
                           unpack, colors);
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
      draw_rgba_pixels(ctx, x, y, width, height, colorFormat, colorType,
                       unpack, colors);
      break;
   default:
      _mesa_problem(ctx, "unexpected format in glDrawDepthPixelsMESA");
   }

   RENDER_FINISH(swrast,ctx);
}
#endif
