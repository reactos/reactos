/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
#include "convolve.h"
#include "histogram.h"
#include "macros.h"
#include "imports.h"
#include "pixel.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_pixeltex.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_texture.h"
#include "s_zoom.h"



/*
 * Determine if there's overlap in an image copy.
 * This test also compensates for the fact that copies are done from
 * bottom to top and overlaps can sometimes be handled correctly
 * without making a temporary image copy.
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
      if ((srcx > dstx + (width * zoomX) + 1) || (srcx + width + 1 < dstx)) {
         return GL_FALSE;
      }
      else if ((srcy < dsty) && (srcy + height < dsty + (height * zoomY))) {
         return GL_FALSE;
      }
      else if ((srcy > dsty) && (srcy + height > dsty + (height * zoomY))) {
         return GL_FALSE;
      }
      else {
         return GL_TRUE;
      }
   }
}



/*
 * RGBA copypixels with convolution.
 */
static void
copy_conv_rgba_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                      GLint width, GLint height, GLint destx, GLint desty)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLboolean quick_draw;
   GLint row;
   GLboolean changeBuffer;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLuint transferOps = ctx->_ImageTransferState;
   GLfloat *dest, *tmpImage, *convImage;
   struct sw_span span;

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_RGBA);

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (ctx->Fog.Enabled)
      _swrast_span_default_fog(ctx, &span);


   if (SWRAST_CONTEXT(ctx)->_RasterMask == 0
       && !zoom
       && destx >= 0
       && destx + width <= (GLint) ctx->DrawBuffer->Width) {
      quick_draw = GL_TRUE;
   }
   else {
      quick_draw = GL_FALSE;
   }

   /* If read and draw buffer are different we must do buffer switching */
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer
               || ctx->DrawBuffer != ctx->ReadBuffer;


   /* allocate space for GLfloat image */
   tmpImage = (GLfloat *) MALLOC(width * height * 4 * sizeof(GLfloat));
   if (!tmpImage) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
      return;
   }
   convImage = (GLfloat *) MALLOC(width * height * 4 * sizeof(GLfloat));
   if (!convImage) {
      FREE(tmpImage);
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
      return;
   }

   dest = tmpImage;

   if (changeBuffer) {
      /* choose the read buffer */
      _swrast_use_read_buffer(ctx);
   }

   /* read source image */
   dest = tmpImage;
   for (row = 0; row < height; row++) {
      GLchan rgba[MAX_WIDTH][4];
      GLint i;
      _swrast_read_rgba_span(ctx, ctx->ReadBuffer, width, srcx, srcy + row, rgba);
      /* convert GLchan to GLfloat */
      for (i = 0; i < width; i++) {
         *dest++ = (GLfloat) rgba[i][RCOMP] * (1.0F / CHAN_MAXF);
         *dest++ = (GLfloat) rgba[i][GCOMP] * (1.0F / CHAN_MAXF);
         *dest++ = (GLfloat) rgba[i][BCOMP] * (1.0F / CHAN_MAXF);
         *dest++ = (GLfloat) rgba[i][ACOMP] * (1.0F / CHAN_MAXF);
      }
   }

   if (changeBuffer) {
      /* restore default src/dst buffer */
      _swrast_use_draw_buffer(ctx);
   }

   /* do image transfer ops up until convolution */
   for (row = 0; row < height; row++) {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) (tmpImage + row * width * 4);

      /* scale & bias */
      if (transferOps & IMAGE_SCALE_BIAS_BIT) {
         _mesa_scale_and_bias_rgba(ctx, width, rgba,
                                   ctx->Pixel.RedScale, ctx->Pixel.GreenScale,
                                   ctx->Pixel.BlueScale, ctx->Pixel.AlphaScale,
                                   ctx->Pixel.RedBias, ctx->Pixel.GreenBias,
                                   ctx->Pixel.BlueBias, ctx->Pixel.AlphaBias);
      }
      /* color map lookup */
      if (transferOps & IMAGE_MAP_COLOR_BIT) {
         _mesa_map_rgba(ctx, width, rgba);
      }
      /* GL_COLOR_TABLE lookup */
      if (transferOps & IMAGE_COLOR_TABLE_BIT) {
         _mesa_lookup_rgba(&ctx->ColorTable, width, rgba);
      }
   }

   /* do convolution */
   if (ctx->Pixel.Convolution2DEnabled) {
      _mesa_convolve_2d_image(ctx, &width, &height, tmpImage, convImage);
   }
   else {
      ASSERT(ctx->Pixel.Separable2DEnabled);
      _mesa_convolve_sep_image(ctx, &width, &height, tmpImage, convImage);
   }
   FREE(tmpImage);

   /* do remaining image transfer ops */
   for (row = 0; row < height; row++) {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) (convImage + row * width * 4);

      /* GL_POST_CONVOLUTION_COLOR_TABLE lookup */
      if (transferOps & IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT) {
         _mesa_lookup_rgba(&ctx->PostConvolutionColorTable, width, rgba);
      }
      /* color matrix */
      if (transferOps & IMAGE_COLOR_MATRIX_BIT) {
         _mesa_transform_rgba(ctx, width, rgba);
      }
      /* GL_POST_COLOR_MATRIX_COLOR_TABLE lookup */
      if (transferOps & IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT) {
         _mesa_lookup_rgba(&ctx->PostColorMatrixColorTable, width, rgba);
      }
      /* update histogram count */
      if (transferOps & IMAGE_HISTOGRAM_BIT) {
         _mesa_update_histogram(ctx, width, (CONST GLfloat (*)[4]) rgba);
      }
      /* update min/max */
      if (transferOps & IMAGE_MIN_MAX_BIT) {
         _mesa_update_minmax(ctx, width, (CONST GLfloat (*)[4]) rgba);
      }
   }

   for (row = 0; row < height; row++) {
      const GLfloat *src = convImage + row * width * 4;
      GLint i, dy;

      /* clamp to [0,1] and convert float back to chan */
      for (i = 0; i < width; i++) {
         GLint r = (GLint) (src[i * 4 + RCOMP] * CHAN_MAXF);
         GLint g = (GLint) (src[i * 4 + GCOMP] * CHAN_MAXF);
         GLint b = (GLint) (src[i * 4 + BCOMP] * CHAN_MAXF);
         GLint a = (GLint) (src[i * 4 + ACOMP] * CHAN_MAXF);
         span.array->rgba[i][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
         span.array->rgba[i][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
         span.array->rgba[i][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
         span.array->rgba[i][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
      }

      if (ctx->Pixel.PixelTextureEnabled && ctx->Texture._EnabledUnits) {
         span.end = width;
         _swrast_pixel_texture(ctx, &span);
      }

      /* write row to framebuffer */

      dy = desty + row;
      if (quick_draw && dy >= 0 && dy < (GLint) ctx->DrawBuffer->Height) {
         (*swrast->Driver.WriteRGBASpan)( ctx, width, destx, dy,
		       (const GLchan (*)[4])span.array->rgba, NULL );
      }
      else if (zoom) {
         span.x = destx;
         span.y = dy;
         span.end = width;
         _swrast_write_zoomed_rgba_span(ctx, &span, 
                                     (CONST GLchan (*)[4])span.array->rgba,
                                     desty, 0);
      }
      else {
         span.x = destx;
         span.y = dy;
         span.end = width;
         _swrast_write_rgba_span(ctx, &span);
      }
   }

   FREE(convImage);
}


/*
 * RGBA copypixels
 */
static void
copy_rgba_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                 GLint width, GLint height, GLint destx, GLint desty)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLchan *tmpImage,*p;
   GLboolean quick_draw;
   GLint sy, dy, stepy, j;
   GLboolean changeBuffer;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   const GLuint transferOps = ctx->_ImageTransferState;
   struct sw_span span;

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_RGBA);

   if (ctx->Pixel.Convolution2DEnabled || ctx->Pixel.Separable2DEnabled) {
      copy_conv_rgba_pixels(ctx, srcx, srcy, width, height, destx, desty);
      return;
   }

   /* Determine if copy should be done bottom-to-top or top-to-bottom */
   if (srcy < desty) {
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

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (ctx->Fog.Enabled)
      _swrast_span_default_fog(ctx, &span);

   if (SWRAST_CONTEXT(ctx)->_RasterMask == 0
       && !zoom
       && destx >= 0
       && destx + width <= (GLint) ctx->DrawBuffer->Width) {
      quick_draw = GL_TRUE;
   }
   else {
      quick_draw = GL_FALSE;
   }

   /* If read and draw buffer are different we must do buffer switching */
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer
                  || ctx->DrawBuffer != ctx->ReadBuffer;

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLchan *) MALLOC(width * height * sizeof(GLchan) * 4);
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      /* setup source */
      if (changeBuffer)
         _swrast_use_read_buffer(ctx);
      /* read the source image */
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_rgba_span( ctx, ctx->ReadBuffer, width, srcx, ssy,
                            (GLchan (*)[4]) p );
         p += width * 4;
      }
      p = tmpImage;
      /* restore dest */
      if (changeBuffer) {
         _swrast_use_draw_buffer(ctx);
         changeBuffer = GL_FALSE;
      }
   }
   else {
      tmpImage = NULL;  /* silence compiler warnings */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      /* Get source pixels */
      if (overlapping) {
         /* get from buffered image */
         ASSERT(width < MAX_WIDTH);
         MEMCPY(span.array->rgba, p, width * sizeof(GLchan) * 4);
         p += width * 4;
      }
      else {
         /* get from framebuffer */
         if (changeBuffer)
            _swrast_use_read_buffer(ctx);
         ASSERT(width < MAX_WIDTH);
         _swrast_read_rgba_span( ctx, ctx->ReadBuffer, width, srcx, sy,
                               span.array->rgba );
         if (changeBuffer)
            _swrast_use_draw_buffer(ctx);
      }

      if (transferOps) {
         const GLfloat scale = (1.0F / CHAN_MAXF);
         GLint k;
         DEFMARRAY(GLfloat, rgbaFloat, MAX_WIDTH, 4);  /* mac 32k limitation */
         CHECKARRAY(rgbaFloat, return);

         /* convert chan to float */
         for (k = 0; k < width; k++) {
            rgbaFloat[k][RCOMP] = (GLfloat) span.array->rgba[k][RCOMP] * scale;
            rgbaFloat[k][GCOMP] = (GLfloat) span.array->rgba[k][GCOMP] * scale;
            rgbaFloat[k][BCOMP] = (GLfloat) span.array->rgba[k][BCOMP] * scale;
            rgbaFloat[k][ACOMP] = (GLfloat) span.array->rgba[k][ACOMP] * scale;
         }
         /* scale & bias */
         if (transferOps & IMAGE_SCALE_BIAS_BIT) {
            _mesa_scale_and_bias_rgba(ctx, width, rgbaFloat,
                                   ctx->Pixel.RedScale, ctx->Pixel.GreenScale,
                                   ctx->Pixel.BlueScale, ctx->Pixel.AlphaScale,
                                   ctx->Pixel.RedBias, ctx->Pixel.GreenBias,
                                   ctx->Pixel.BlueBias, ctx->Pixel.AlphaBias);
         }
         /* color map lookup */
         if (transferOps & IMAGE_MAP_COLOR_BIT) {
            _mesa_map_rgba(ctx, width, rgbaFloat);
         }
         /* GL_COLOR_TABLE lookup */
         if (transferOps & IMAGE_COLOR_TABLE_BIT) {
            _mesa_lookup_rgba(&ctx->ColorTable, width, rgbaFloat);
         }
         /* convolution */
         if (transferOps & IMAGE_CONVOLUTION_BIT) {
            _mesa_problem(ctx, "Convolution should not be enabled in copy_rgba_pixels()");
            return;
         }
         /* GL_POST_CONVOLUTION_RED/GREEN/BLUE/ALPHA_SCALE/BIAS */
         if (transferOps & IMAGE_POST_CONVOLUTION_SCALE_BIAS) {
            _mesa_scale_and_bias_rgba(ctx, width, rgbaFloat,
                                      ctx->Pixel.PostConvolutionScale[RCOMP],
                                      ctx->Pixel.PostConvolutionScale[GCOMP],
                                      ctx->Pixel.PostConvolutionScale[BCOMP],
                                      ctx->Pixel.PostConvolutionScale[ACOMP],
                                      ctx->Pixel.PostConvolutionBias[RCOMP],
                                      ctx->Pixel.PostConvolutionBias[GCOMP],
                                      ctx->Pixel.PostConvolutionBias[BCOMP],
                                      ctx->Pixel.PostConvolutionBias[ACOMP]);
         }
         /* GL_POST_CONVOLUTION_COLOR_TABLE lookup */
         if (transferOps & IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT) {
            _mesa_lookup_rgba(&ctx->PostConvolutionColorTable, width, rgbaFloat);
         }
         /* color matrix */
         if (transferOps & IMAGE_COLOR_MATRIX_BIT) {
            _mesa_transform_rgba(ctx, width, rgbaFloat);
         }
         /* GL_POST_COLOR_MATRIX_COLOR_TABLE lookup */
         if (transferOps & IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT) {
            _mesa_lookup_rgba(&ctx->PostColorMatrixColorTable, width, rgbaFloat);
         }
         /* update histogram count */
         if (transferOps & IMAGE_HISTOGRAM_BIT) {
            _mesa_update_histogram(ctx, width, (CONST GLfloat (*)[4]) rgbaFloat);
         }
         /* update min/max */
         if (transferOps & IMAGE_MIN_MAX_BIT) {
            _mesa_update_minmax(ctx, width, (CONST GLfloat (*)[4]) rgbaFloat);
         }
         /* clamp to [0,1] and convert float back to chan */
         for (k = 0; k < width; k++) {
            GLint r = (GLint) (rgbaFloat[k][RCOMP] * CHAN_MAXF);
            GLint g = (GLint) (rgbaFloat[k][GCOMP] * CHAN_MAXF);
            GLint b = (GLint) (rgbaFloat[k][BCOMP] * CHAN_MAXF);
            GLint a = (GLint) (rgbaFloat[k][ACOMP] * CHAN_MAXF);
            span.array->rgba[k][RCOMP] = (GLchan) CLAMP(r, 0, CHAN_MAX);
            span.array->rgba[k][GCOMP] = (GLchan) CLAMP(g, 0, CHAN_MAX);
            span.array->rgba[k][BCOMP] = (GLchan) CLAMP(b, 0, CHAN_MAX);
            span.array->rgba[k][ACOMP] = (GLchan) CLAMP(a, 0, CHAN_MAX);
         }
         UNDEFARRAY(rgbaFloat);  /* mac 32k limitation */
      }

      if (ctx->Pixel.PixelTextureEnabled && ctx->Texture._EnabledUnits) {
         span.end = width;
         _swrast_pixel_texture(ctx, &span);
      }

      /* Write color span */
      if (quick_draw && dy >= 0 && dy < (GLint) ctx->DrawBuffer->Height) {
         (*swrast->Driver.WriteRGBASpan)( ctx, width, destx, dy,
                                 (const GLchan (*)[4])span.array->rgba, NULL );
      }
      else if (zoom) {
         span.x = destx;
         span.y = dy;
         span.end = width;
         _swrast_write_zoomed_rgba_span(ctx, &span,
                                     (CONST GLchan (*)[4]) span.array->rgba,
                                     desty, 0);
      }
      else {
         span.x = destx;
         span.y = dy;
         span.end = width;
         _swrast_write_rgba_span(ctx, &span);
      }
   }

   if (overlapping)
      FREE(tmpImage);
}


static void
copy_ci_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                GLint width, GLint height,
                GLint destx, GLint desty )
{
   GLuint *tmpImage,*p;
   GLint sy, dy, stepy;
   GLint j;
   GLboolean changeBuffer;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLboolean shift_or_offset = ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset;
   GLint overlapping;
   struct sw_span span;

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_INDEX);

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
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

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (ctx->Fog.Enabled)
      _swrast_span_default_fog(ctx, &span);

   /* If read and draw buffer are different we must do buffer switching */
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer
               || ctx->DrawBuffer != ctx->ReadBuffer;

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLuint *) MALLOC(width * height * sizeof(GLuint));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      /* setup source */
      if (changeBuffer)
         _swrast_use_read_buffer(ctx);
      /* read the image */
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_index_span( ctx, ctx->ReadBuffer, width, srcx, ssy, p );
         p += width;
      }
      p = tmpImage;
      /* restore to draw buffer */
      if (changeBuffer) {
         _swrast_use_draw_buffer(ctx);
         changeBuffer = GL_FALSE;
      }
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      /* Get color indexes */
      if (overlapping) {
         MEMCPY(span.array->index, p, width * sizeof(GLuint));
         p += width;
      }
      else {
         if (changeBuffer)
            _swrast_use_read_buffer(ctx);
         _swrast_read_index_span( ctx, ctx->ReadBuffer, width, srcx, sy,
                                span.array->index );
         if (changeBuffer)
            _swrast_use_draw_buffer(ctx);
      }

      /* Apply shift, offset, look-up table */
      if (shift_or_offset) {
         _mesa_shift_and_offset_ci( ctx, width, span.array->index );
      }
      if (ctx->Pixel.MapColorFlag) {
         _mesa_map_ci( ctx, width, span.array->index );
      }

      /* write color indexes */
      span.x = destx;
      span.y = dy;
      span.end = width;
      if (zoom)
         _swrast_write_zoomed_index_span(ctx, &span, desty, 0);
      else
         _swrast_write_index_span(ctx, &span);
   }

   if (overlapping)
      FREE(tmpImage);
}



/*
 * TODO: Optimize!!!!
 */
static void
copy_depth_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                   GLint width, GLint height,
                   GLint destx, GLint desty )
{
   GLfloat *p, *tmpImage;
   GLint sy, dy, stepy;
   GLint i, j;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   struct sw_span span;

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_Z);

   if (!ctx->Visual.depthBits) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
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

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   _swrast_span_default_color(ctx, &span);
   if (ctx->Fog.Enabled)
      _swrast_span_default_fog(ctx, &span);

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLfloat *) MALLOC(width * height * sizeof(GLfloat));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_depth_span_float(ctx, width, srcx, ssy, p);
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
         MEMCPY(depth, p, width * sizeof(GLfloat));
         p += width;
      }
      else {
         _swrast_read_depth_span_float(ctx, width, srcx, sy, depth);
      }

      /* apply scale and bias */
      for (i = 0; i < width; i++) {
         GLfloat d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         span.array->z[i] = (GLdepth) (CLAMP(d, 0.0F, 1.0F) * ctx->DepthMax);
      }

      /* write depth values */
      span.x = destx;
      span.y = dy;
      span.end = width;
      if (ctx->Visual.rgbMode) {
         if (zoom)
            _swrast_write_zoomed_rgba_span( ctx, &span, 
                            (const GLchan (*)[4])span.array->rgba, desty, 0 );
         else
            _swrast_write_rgba_span(ctx, &span);
      }
      else {
         if (zoom)
            _swrast_write_zoomed_index_span( ctx, &span, desty, 0 );
         else
            _swrast_write_index_span(ctx, &span);
      }
   }

   if (overlapping)
      FREE(tmpImage);
}



static void
copy_stencil_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                     GLint width, GLint height,
                     GLint destx, GLint desty )
{
   GLint sy, dy, stepy;
   GLint j;
   GLstencil *p, *tmpImage;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLboolean shift_or_offset = ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset;
   GLint overlapping;

   if (!ctx->Visual.stencilBits) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy < desty) {
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

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLstencil *) MALLOC(width * height * sizeof(GLstencil));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_stencil_span( ctx, width, srcx, ssy, p );
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
         MEMCPY(stencil, p, width * sizeof(GLstencil));
         p += width;
      }
      else {
         _swrast_read_stencil_span( ctx, width, srcx, sy, stencil );
      }

      /* Apply shift, offset, look-up table */
      if (shift_or_offset) {
         _mesa_shift_and_offset_stencil( ctx, width, stencil );
      }
      if (ctx->Pixel.MapStencilFlag) {
         _mesa_map_stencil( ctx, width, stencil );
      }

      /* Write stencil values */
      if (zoom) {
         _swrast_write_zoomed_stencil_span( ctx, width, destx, dy,
                                          stencil, desty, 0 );
      }
      else {
         _swrast_write_stencil_span( ctx, width, destx, dy, stencil );
      }
   }

   if (overlapping)
      FREE(tmpImage);
}



void
_swrast_CopyPixels( GLcontext *ctx,
		    GLint srcx, GLint srcy, GLsizei width, GLsizei height,
		    GLint destx, GLint desty,
		    GLenum type )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   RENDER_START(swrast,ctx);
      
   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (type == GL_COLOR && ctx->Visual.rgbMode) {
      copy_rgba_pixels( ctx, srcx, srcy, width, height, destx, desty );
   }
   else if (type == GL_COLOR && !ctx->Visual.rgbMode) {
      copy_ci_pixels( ctx, srcx, srcy, width, height, destx, desty );
   }
   else if (type == GL_DEPTH) {
      copy_depth_pixels( ctx, srcx, srcy, width, height, destx, desty );
   }
   else if (type == GL_STENCIL) {
      copy_stencil_pixels( ctx, srcx, srcy, width, height, destx, desty );
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glCopyPixels" );
   }

   RENDER_FINISH(swrast,ctx);
}
