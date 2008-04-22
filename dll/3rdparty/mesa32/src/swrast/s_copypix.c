/*
 * Mesa 3-D graphics library
 * Version:  7.0.2
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


#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "convolve.h"
#include "histogram.h"
#include "image.h"
#include "macros.h"
#include "imports.h"
#include "pixel.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_zoom.h"



/**
 * Determine if there's overlap in an image copy.
 * This test also compensates for the fact that copies are done from
 * bottom to top and overlaps can sometimes be handled correctly
 * without making a temporary image copy.
 * \return GL_TRUE if the regions overlap, GL_FALSE otherwise.
 */
static GLboolean
regions_overlap(GLint srcx, GLint srcy,
                GLint dstx, GLint dsty,
                GLint width, GLint height,
                GLfloat zoomX, GLfloat zoomY)
{
   if (zoomX == 1.0 && zoomY == 1.0) {
      /* no zoom */
      if (srcx >= dstx + width || (srcx + width <= dstx)) {
         return GL_FALSE;
      }
      else if (srcy < dsty) { /* this is OK */
         return GL_FALSE;
      }
      else if (srcy > dsty + height) {
         return GL_FALSE;
      }
      else {
         return GL_TRUE;
      }
   }
   else {
      /* add one pixel of slop when zooming, just to be safe */
      if (srcx > (dstx + ((zoomX > 0.0F) ? (width * zoomX + 1.0F) : 0.0F))) {
         /* src is completely right of dest */
         return GL_FALSE;
      }
      else if (srcx + width + 1.0F < dstx + ((zoomX > 0.0F) ? 0.0F : (width * zoomX))) {
         /* src is completely left of dest */
         return GL_FALSE;
      }
      else if ((srcy < dsty) && (srcy + height < dsty + (height * zoomY))) {
         /* src is completely below dest */
         return GL_FALSE;
      }
      else if ((srcy > dsty) && (srcy + height > dsty + (height * zoomY))) {
         /* src is completely above dest */
         return GL_FALSE;
      }
      else {
         return GL_TRUE;
      }
   }
}


/**
 * RGBA copypixels with convolution.
 */
static void
copy_conv_rgba_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                      GLint width, GLint height, GLint destx, GLint desty)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLint row;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLbitfield transferOps = ctx->_ImageTransferState;
   const GLboolean sink = (ctx->Pixel.MinMaxEnabled && ctx->MinMax.Sink)
      || (ctx->Pixel.HistogramEnabled && ctx->Histogram.Sink);
   GLfloat *dest, *tmpImage, *convImage;
   SWspan span;

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_RGBA);

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (swrast->_FogEnabled)
      _swrast_span_default_fog(ctx, &span);
   _swrast_span_default_secondary_color(ctx, &span);

   /* allocate space for GLfloat image */
   tmpImage = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
   if (!tmpImage) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
      return;
   }
   convImage = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
   if (!convImage) {
      _mesa_free(tmpImage);
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
      return;
   }

   /* read source image as float/RGBA */
   dest = tmpImage;
   for (row = 0; row < height; row++) {
      _swrast_read_rgba_span(ctx, ctx->ReadBuffer->_ColorReadBuffer,
                             width, srcx, srcy + row, GL_FLOAT, dest);
      dest += 4 * width;
   }

   /* do the image transfer ops which preceed convolution */
   for (row = 0; row < height; row++) {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) (tmpImage + row * width * 4);
      _mesa_apply_rgba_transfer_ops(ctx,
                                    transferOps & IMAGE_PRE_CONVOLUTION_BITS,
                                    width, rgba);
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

   /* do remaining post-convolution image transfer ops */
   for (row = 0; row < height; row++) {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) (convImage + row * width * 4);
      _mesa_apply_rgba_transfer_ops(ctx,
                                    transferOps & IMAGE_POST_CONVOLUTION_BITS,
                                    width, rgba);
   }

   if (!sink) {
      /* write the new image */
      for (row = 0; row < height; row++) {
         const GLfloat *src = convImage + row * width * 4;
         GLvoid *rgba = (GLvoid *) span.array->attribs[FRAG_ATTRIB_COL0];

         /* copy convolved colors into span array */
         _mesa_memcpy(rgba, src, width * 4 * sizeof(GLfloat));

         /* write span */
         span.x = destx;
         span.y = desty + row;
         span.end = width;
         span.array->ChanType = GL_FLOAT;
         if (zoom) {
            _swrast_write_zoomed_rgba_span(ctx, destx, desty, &span, rgba);
         }
         else {
            _swrast_write_rgba_span(ctx, &span);
         }
      }
      /* restore this */
      span.array->ChanType = CHAN_TYPE;
   }

   _mesa_free(convImage);
}


/**
 * RGBA copypixels
 */
static void
copy_rgba_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                 GLint width, GLint height, GLint destx, GLint desty)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLfloat *tmpImage, *p;
   GLint sy, dy, stepy, row;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   GLuint transferOps = ctx->_ImageTransferState;
   SWspan span;

   if (!ctx->ReadBuffer->_ColorReadBuffer) {
      /* no readbuffer - OK */
      return;
   }

   if (ctx->Pixel.Convolution2DEnabled || ctx->Pixel.Separable2DEnabled) {
      copy_conv_rgba_pixels(ctx, srcx, srcy, width, height, destx, desty);
      return;
   }
   else if (ctx->Pixel.Convolution1DEnabled) {
      /* make sure we don't apply 1D convolution */
      transferOps &= ~(IMAGE_CONVOLUTION_BIT |
                       IMAGE_POST_CONVOLUTION_SCALE_BIAS);
   }

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   /* Determine if copy should be done bottom-to-top or top-to-bottom */
   if (!overlapping && srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_RGBA);
   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (swrast->_FogEnabled)
      _swrast_span_default_fog(ctx, &span);
   _swrast_span_default_secondary_color(ctx, &span);

   if (overlapping) {
      tmpImage = (GLfloat *) _mesa_malloc(width * height * sizeof(GLfloat) * 4);
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      /* read the source image as RGBA/float */
      p = tmpImage;
      for (row = 0; row < height; row++) {
         _swrast_read_rgba_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                 width, srcx, sy + row, GL_FLOAT, p );
         p += width * 4;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warnings */
      p = NULL;
   }

   ASSERT(width < MAX_WIDTH);

   for (row = 0; row < height; row++, sy += stepy, dy += stepy) {
      GLvoid *rgba = span.array->attribs[FRAG_ATTRIB_COL0];

      /* Get row/span of source pixels */
      if (overlapping) {
         /* get from buffered image */
         _mesa_memcpy(rgba, p, width * sizeof(GLfloat) * 4);
         p += width * 4;
      }
      else {
         /* get from framebuffer */
         _swrast_read_rgba_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                 width, srcx, sy, GL_FLOAT, rgba );
      }

      if (transferOps) {
         _mesa_apply_rgba_transfer_ops(ctx, transferOps, width,
                                       (GLfloat (*)[4]) rgba);
      }

      /* Write color span */
      span.x = destx;
      span.y = dy;
      span.end = width;
      span.array->ChanType = GL_FLOAT;
      if (zoom) {
         _swrast_write_zoomed_rgba_span(ctx, destx, desty, &span, rgba);
      }
      else {
         _swrast_write_rgba_span(ctx, &span);
      }
   }

   span.array->ChanType = CHAN_TYPE; /* restore */

   if (overlapping)
      _mesa_free(tmpImage);
}


static void
copy_ci_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                GLint width, GLint height,
                GLint destx, GLint desty )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint *tmpImage,*p;
   GLint sy, dy, stepy;
   GLint j;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   SWspan span;

   if (!ctx->ReadBuffer->_ColorReadBuffer) {
      /* no readbuffer - OK */
      return;
   }

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_INDEX);

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (!overlapping && srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (swrast->_FogEnabled)
      _swrast_span_default_fog(ctx, &span);

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLuint *) _mesa_malloc(width * height * sizeof(GLuint));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      /* read the image */
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_index_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                  width, srcx, ssy, p );
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      /* Get color indexes */
      if (overlapping) {
         _mesa_memcpy(span.array->index, p, width * sizeof(GLuint));
         p += width;
      }
      else {
         _swrast_read_index_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                  width, srcx, sy, span.array->index );
      }

      if (ctx->_ImageTransferState)
         _mesa_apply_ci_transfer_ops(ctx, ctx->_ImageTransferState,
                                     width, span.array->index);

      /* write color indexes */
      span.x = destx;
      span.y = dy;
      span.end = width;
      if (zoom)
         _swrast_write_zoomed_index_span(ctx, destx, desty, &span);
      else
         _swrast_write_index_span(ctx, &span);
   }

   if (overlapping)
      _mesa_free(tmpImage);
}


/**
 * Convert floating point Z values to integer Z values with pixel transfer's
 * Z scale and bias.
 */
static void
scale_and_bias_z(GLcontext *ctx, GLuint width,
                 const GLfloat depth[], GLuint z[])
{
   const GLuint depthMax = ctx->DrawBuffer->_DepthMax;
   GLuint i;

   if (depthMax <= 0xffffff &&
       ctx->Pixel.DepthScale == 1.0 &&
       ctx->Pixel.DepthBias == 0.0) {
      /* no scale or bias and no clamping and no worry of overflow */
      const GLfloat depthMaxF = ctx->DrawBuffer->_DepthMaxF;
      for (i = 0; i < width; i++) {
         z[i] = (GLuint) (depth[i] * depthMaxF);
      }
   }
   else {
      /* need to be careful with overflow */
      const GLdouble depthMaxF = ctx->DrawBuffer->_DepthMaxF;
      for (i = 0; i < width; i++) {
         GLdouble d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         d = CLAMP(d, 0.0, 1.0) * depthMaxF;
         if (d >= depthMaxF)
            z[i] = depthMax;
         else
            z[i] = (GLuint) d;
      }
   }
}



/*
 * TODO: Optimize!!!!
 */
static void
copy_depth_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                   GLint width, GLint height,
                   GLint destx, GLint desty )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *readRb = fb->_DepthBuffer;
   GLfloat *p, *tmpImage;
   GLint sy, dy, stepy;
   GLint j;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   SWspan span;

   if (!readRb) {
      /* no readbuffer - OK */
      return;
   }

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_Z);

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (!overlapping && srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   _swrast_span_default_color(ctx, &span);
   _swrast_span_default_secondary_color(ctx, &span);
   if (swrast->_FogEnabled)
      _swrast_span_default_fog(ctx, &span);

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLfloat *) _mesa_malloc(width * height * sizeof(GLfloat));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_depth_span_float(ctx, readRb, width, srcx, ssy, p);
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      GLfloat depth[MAX_WIDTH];
      /* get depth values */
      if (overlapping) {
         _mesa_memcpy(depth, p, width * sizeof(GLfloat));
         p += width;
      }
      else {
         _swrast_read_depth_span_float(ctx, readRb, width, srcx, sy, depth);
      }

      /* apply scale and bias */
      scale_and_bias_z(ctx, width, depth, span.array->z);

      /* write depth values */
      span.x = destx;
      span.y = dy;
      span.end = width;
      if (fb->Visual.rgbMode) {
         if (zoom)
            _swrast_write_zoomed_depth_span(ctx, destx, desty, &span);
         else
            _swrast_write_rgba_span(ctx, &span);
      }
      else {
         if (zoom)
            _swrast_write_zoomed_depth_span(ctx, destx, desty, &span);
         else
            _swrast_write_index_span(ctx, &span);
      }
   }

   if (overlapping)
      _mesa_free(tmpImage);
}



static void
copy_stencil_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                     GLint width, GLint height,
                     GLint destx, GLint desty )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   GLint sy, dy, stepy;
   GLint j;
   GLstencil *p, *tmpImage;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;

   if (!rb) {
      /* no readbuffer - OK */
      return;
   }

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (!overlapping && srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLstencil *) _mesa_malloc(width * height * sizeof(GLstencil));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_stencil_span( ctx, rb, width, srcx, ssy, p );
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      GLstencil stencil[MAX_WIDTH];

      /* Get stencil values */
      if (overlapping) {
         _mesa_memcpy(stencil, p, width * sizeof(GLstencil));
         p += width;
      }
      else {
         _swrast_read_stencil_span( ctx, rb, width, srcx, sy, stencil );
      }

      _mesa_apply_stencil_transfer_ops(ctx, width, stencil);

      /* Write stencil values */
      if (zoom) {
         _swrast_write_zoomed_stencil_span(ctx, destx, desty, width,
                                           destx, dy, stencil);
      }
      else {
         _swrast_write_stencil_span( ctx, width, destx, dy, stencil );
      }
   }

   if (overlapping)
      _mesa_free(tmpImage);
}


/**
 * This isn't terribly efficient.  If a driver really has combined
 * depth/stencil buffers the driver should implement an optimized
 * CopyPixels function.
 */
static void
copy_depth_stencil_pixels(GLcontext *ctx,
                          const GLint srcX, const GLint srcY,
                          const GLint width, const GLint height,
                          const GLint destX, const GLint destY)
{
   struct gl_renderbuffer *stencilReadRb, *depthReadRb, *depthDrawRb;
   GLint sy, dy, stepy;
   GLint j;
   GLstencil *tempStencilImage = NULL, *stencilPtr = NULL;
   GLfloat *tempDepthImage = NULL, *depthPtr = NULL;
   const GLfloat depthScale = ctx->DrawBuffer->_DepthMaxF;
   const GLuint stencilMask = ctx->Stencil.WriteMask[0];
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLboolean scaleOrBias
      = ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0;
   GLint overlapping;

   depthDrawRb = ctx->DrawBuffer->_DepthBuffer;
   depthReadRb = ctx->ReadBuffer->_DepthBuffer;
   stencilReadRb = ctx->ReadBuffer->_StencilBuffer;

   ASSERT(depthDrawRb);
   ASSERT(depthReadRb);
   ASSERT(stencilReadRb);

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcX, srcY, destX, destY, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (!overlapping && srcY < destY) {
      /* top-down  max-to-min */
      sy = srcY + height - 1;
      dy = destY + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcY;
      dy = destY;
      stepy = 1;
   }

   if (overlapping) {
      GLint ssy = sy;

      if (stencilMask != 0x0) {
         tempStencilImage
            = (GLstencil *) _mesa_malloc(width * height * sizeof(GLstencil));
         if (!tempStencilImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
            return;
         }

         /* get copy of stencil pixels */
         stencilPtr = tempStencilImage;
         for (j = 0; j < height; j++, ssy += stepy) {
            _swrast_read_stencil_span(ctx, stencilReadRb,
                                      width, srcX, ssy, stencilPtr);
            stencilPtr += width;
         }
         stencilPtr = tempStencilImage;
      }

      if (ctx->Depth.Mask) {
         tempDepthImage
            = (GLfloat *) _mesa_malloc(width * height * sizeof(GLfloat));
         if (!tempDepthImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
            _mesa_free(tempStencilImage);
            return;
         }

         /* get copy of depth pixels */
         depthPtr = tempDepthImage;
         for (j = 0; j < height; j++, ssy += stepy) {
            _swrast_read_depth_span_float(ctx, depthReadRb,
                                          width, srcX, ssy, depthPtr);
            depthPtr += width;
         }
         depthPtr = tempDepthImage;
      }
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      if (stencilMask != 0x0) {
         GLstencil stencil[MAX_WIDTH];

         /* Get stencil values */
         if (overlapping) {
            _mesa_memcpy(stencil, stencilPtr, width * sizeof(GLstencil));
            stencilPtr += width;
         }
         else {
            _swrast_read_stencil_span(ctx, stencilReadRb,
                                      width, srcX, sy, stencil);
         }

         _mesa_apply_stencil_transfer_ops(ctx, width, stencil);

         /* Write values */
         if (zoom) {
            _swrast_write_zoomed_stencil_span(ctx, destX, destY, width,
                                              destX, dy, stencil);
         }
         else {
            _swrast_write_stencil_span( ctx, width, destX, dy, stencil );
         }
      }

      if (ctx->Depth.Mask) {
         GLfloat depth[MAX_WIDTH];
         GLuint zVals32[MAX_WIDTH];
         GLushort zVals16[MAX_WIDTH];
         GLvoid *zVals;
         GLuint zBytes;

         /* get depth values */
         if (overlapping) {
            _mesa_memcpy(depth, depthPtr, width * sizeof(GLfloat));
            depthPtr += width;
         }
         else {
            _swrast_read_depth_span_float(ctx, depthReadRb,
                                          width, srcX, sy, depth);
         }

         /* scale & bias */
         if (scaleOrBias) {
            _mesa_scale_and_bias_depth(ctx, width, depth);
         }
         /* convert to integer Z values */
         if (depthDrawRb->DataType == GL_UNSIGNED_SHORT) {
            GLint k;
            for (k = 0; k < width; k++)
               zVals16[k] = (GLushort) (depth[k] * depthScale);
            zVals = zVals16;
            zBytes = 2;
         }
         else {
            GLint k;
            for (k = 0; k < width; k++)
               zVals32[k] = (GLuint) (depth[k] * depthScale);
            zVals = zVals32;
            zBytes = 4;
         }

         /* Write values */
         if (zoom) {
            _swrast_write_zoomed_z_span(ctx, destX, destY, width,
                                        destX, dy, zVals);
         }
         else {
            _swrast_put_row(ctx, depthDrawRb, width, destX, dy, zVals, zBytes);
         }
      }
   }

   if (tempStencilImage)
      _mesa_free(tempStencilImage);

   if (tempDepthImage)
      _mesa_free(tempDepthImage);
}



/**
 * Try to do a fast copy pixels.
 */
static GLboolean
fast_copy_pixels(GLcontext *ctx,
                 GLint srcX, GLint srcY, GLsizei width, GLsizei height,
                 GLint dstX, GLint dstY, GLenum type)
{
   struct gl_framebuffer *srcFb = ctx->ReadBuffer;
   struct gl_framebuffer *dstFb = ctx->DrawBuffer;
   struct gl_renderbuffer *srcRb, *dstRb;
   GLint row, yStep;

   if (SWRAST_CONTEXT(ctx)->_RasterMask != 0x0 ||
       ctx->Pixel.ZoomX != 1.0F ||
       ctx->Pixel.ZoomY != 1.0F ||
       ctx->_ImageTransferState) {
      /* can't handle these */
      return GL_FALSE;
   }

   if (type == GL_COLOR) {
      if (dstFb->_NumColorDrawBuffers[0] != 1)
         return GL_FALSE;
      srcRb = srcFb->_ColorReadBuffer;
      dstRb = dstFb->_ColorDrawBuffers[0][0];
   }
   else if (type == GL_STENCIL) {
      srcRb = srcFb->_StencilBuffer;
      dstRb = dstFb->_StencilBuffer;
   }
   else if (type == GL_DEPTH) {
      srcRb = srcFb->_DepthBuffer;
      dstRb = dstFb->_DepthBuffer;
   }
   else {
      ASSERT(type == GL_DEPTH_STENCIL_EXT);
      /* XXX correct? */
      srcRb = srcFb->Attachment[BUFFER_DEPTH].Renderbuffer;
      dstRb = dstFb->Attachment[BUFFER_DEPTH].Renderbuffer;
   }

   /* src and dst renderbuffers must be same format and type */
   if (!srcRb || !dstRb ||
       srcRb->DataType != dstRb->DataType ||
       srcRb->_BaseFormat != dstRb->_BaseFormat) {
      return GL_FALSE;
   }

   /* clipping not supported */
   if (srcX < 0 || srcX + width > (GLint) srcFb->Width ||
       srcY < 0 || srcY + height > (GLint) srcFb->Height ||
       dstX < dstFb->_Xmin || dstX + width > dstFb->_Xmax ||
       dstY < dstFb->_Ymin || dstY + height > dstFb->_Ymax) {
      return GL_FALSE;
   }

   /* overlapping src/dst doesn't matter, just determine Y direction */
   if (srcY < dstY) {
      /* top-down  max-to-min */
      srcY = srcY + height - 1;
      dstY = dstY + height - 1;
      yStep = -1;
   }
   else {
      /* bottom-up  min-to-max */
      yStep = 1;
   }

   for (row = 0; row < height; row++) {
      GLuint temp[MAX_WIDTH][4];
      srcRb->GetRow(ctx, srcRb, width, srcX, srcY, temp);
      dstRb->PutRow(ctx, dstRb, width, dstX, dstY, temp, NULL);
      srcY += yStep;
      dstY += yStep;
   }

   return GL_TRUE;
}


/**
 * Do software-based glCopyPixels.
 * By time we get here, all parameters will have been error-checked.
 */
void
_swrast_CopyPixels( GLcontext *ctx,
		    GLint srcx, GLint srcy, GLsizei width, GLsizei height,
		    GLint destx, GLint desty, GLenum type )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   RENDER_START(swrast,ctx);
      
   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (!fast_copy_pixels(ctx, srcx, srcy, width, height, destx, desty, type)) {
      switch (type) {
      case GL_COLOR:
         if (ctx->Visual.rgbMode) {
            copy_rgba_pixels( ctx, srcx, srcy, width, height, destx, desty );
         }
         else {
            copy_ci_pixels( ctx, srcx, srcy, width, height, destx, desty );
         }
         break;
      case GL_DEPTH:
         copy_depth_pixels( ctx, srcx, srcy, width, height, destx, desty );
         break;
      case GL_STENCIL:
         copy_stencil_pixels( ctx, srcx, srcy, width, height, destx, desty );
         break;
      case GL_DEPTH_STENCIL_EXT:
         copy_depth_stencil_pixels(ctx, srcx, srcy, width, height, destx, desty);
         break;
      default:
         _mesa_problem(ctx, "unexpected type in _swrast_CopyPixels");
      }
   }

   RENDER_FINISH(swrast,ctx);
}
