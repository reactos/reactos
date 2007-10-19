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


/**
 * Convert GLfloat[n][4] colors to GLchan[n][4].
 * XXX maybe move into image.c
 */
static void
float_span_to_chan(GLuint n, CONST GLfloat in[][4], GLchan out[][4])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      UNCLAMPED_FLOAT_TO_CHAN(out[i][RCOMP], in[i][RCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(out[i][GCOMP], in[i][GCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(out[i][BCOMP], in[i][BCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(out[i][ACOMP], in[i][ACOMP]);
   }
}


/**
 * Convert GLchan[n][4] colors to GLfloat[n][4].
 * XXX maybe move into image.c
 */
static void
chan_span_to_float(GLuint n, CONST GLchan in[][4], GLfloat out[][4])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      out[i][RCOMP] = CHAN_TO_FLOAT(in[i][RCOMP]);
      out[i][GCOMP] = CHAN_TO_FLOAT(in[i][GCOMP]);
      out[i][BCOMP] = CHAN_TO_FLOAT(in[i][BCOMP]);
      out[i][ACOMP] = CHAN_TO_FLOAT(in[i][ACOMP]);
   }
}



/*
 * RGBA copypixels with convolution.
 */
static void
copy_conv_rgba_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                      GLint width, GLint height, GLint destx, GLint desty)
{
   struct gl_renderbuffer *drawRb = NULL;
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
      drawRb = ctx->DrawBuffer->_ColorDrawBuffers[0][0];
   }
   else {
      quick_draw = GL_FALSE;
   }

   /* If read and draw buffer are different we must do buffer switching */
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer[0]
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

   if (changeBuffer) {
      /* choose the read buffer */
      _swrast_use_read_buffer(ctx);
   }

   /* read source image */
   dest = tmpImage;
   for (row = 0; row < height; row++) {
      GLchan rgba[MAX_WIDTH][4];
      /* Read GLchan and convert to GLfloat */
      _swrast_read_rgba_span(ctx, ctx->ReadBuffer->_ColorReadBuffer,
                             width, srcx, srcy + row, rgba);
      chan_span_to_float(width, (CONST GLchan (*)[4]) rgba,
                         (GLfloat (*)[4]) dest);
      dest += 4 * width;
   }

   if (changeBuffer) {
      /* restore default src/dst buffer */
      _swrast_use_draw_buffer(ctx);
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
   FREE(tmpImage);

   /* do remaining post-convolution image transfer ops */
   for (row = 0; row < height; row++) {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) (convImage + row * width * 4);
      _mesa_apply_rgba_transfer_ops(ctx,
                                    transferOps & IMAGE_POST_CONVOLUTION_BITS,
                                    width, rgba);
   }

   /* write the new image */
   for (row = 0; row < height; row++) {
      const GLfloat *src = convImage + row * width * 4;
      GLint dy;

      /* convert floats back to chan */
      float_span_to_chan(width, (const GLfloat (*)[4]) src, span.array->rgba);

      if (ctx->Pixel.PixelTextureEnabled && ctx->Texture._EnabledUnits) {
         span.end = width;
         _swrast_pixel_texture(ctx, &span);
      }

      /* write row to framebuffer */

      dy = desty + row;
      if (quick_draw && dy >= 0 && dy < (GLint) ctx->DrawBuffer->Height) {
         drawRb->PutRow(ctx, drawRb, width, destx, dy, span.array->rgba, NULL);
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
   struct gl_renderbuffer *drawRb;
   GLchan *tmpImage,*p;
   GLboolean quick_draw;
   GLint sy, dy, stepy, j;
   GLboolean changeBuffer;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   const GLuint transferOps = ctx->_ImageTransferState;
   struct sw_span span;

   if (!ctx->ReadBuffer->_ColorReadBuffer) {
      /* no readbuffer - OK */
      return;
   }

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
      drawRb = ctx->DrawBuffer->_ColorDrawBuffers[0][0];
   }
   else {
      quick_draw = GL_FALSE;
      drawRb = NULL;
   }

   /* If read and draw buffer are different we must do buffer switching */
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer[0]
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
         _swrast_read_rgba_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                 width, srcx, ssy, (GLchan (*)[4]) p );
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
         _swrast_read_rgba_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                 width, srcx, sy, span.array->rgba );
         if (changeBuffer)
            _swrast_use_draw_buffer(ctx);
      }

      if (transferOps) {
         DEFMARRAY(GLfloat, rgbaFloat, MAX_WIDTH, 4);  /* mac 32k limitation */
         CHECKARRAY(rgbaFloat, return);

         /* convert to float, transfer, convert back to chan */
         chan_span_to_float(width, (CONST GLchan (*)[4]) span.array->rgba,
                            rgbaFloat);
         _mesa_apply_rgba_transfer_ops(ctx, transferOps, width, rgbaFloat);
         float_span_to_chan(width, (CONST GLfloat (*)[4]) rgbaFloat,
                            span.array->rgba);

         UNDEFARRAY(rgbaFloat);  /* mac 32k limitation */
      }

      if (ctx->Pixel.PixelTextureEnabled && ctx->Texture._EnabledUnits) {
         span.end = width;
         _swrast_pixel_texture(ctx, &span);
      }

      /* Write color span */
      if (quick_draw && dy >= 0 && dy < (GLint) ctx->DrawBuffer->Height) {
         drawRb->PutRow(ctx, drawRb, width, destx, dy, span.array->rgba, NULL);
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

   if (!ctx->ReadBuffer->_ColorReadBuffer) {
      /* no readbuffer - OK */
      return;
   }

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
   changeBuffer = ctx->Pixel.ReadBuffer != ctx->Color.DrawBuffer[0]
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
         _swrast_read_index_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                  width, srcx, ssy, p );
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
         _swrast_read_index_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                  width, srcx, sy, span.array->index );
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
   const GLfloat depthMax = ctx->DrawBuffer->_DepthMaxF;
   struct gl_renderbuffer *readRb
      = ctx->ReadBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLfloat *p, *tmpImage;
   GLint sy, dy, stepy;
   GLint i, j;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   struct sw_span span;

   if (!readRb) {
      /* no readbuffer - OK */
      return;
   }

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
         MEMCPY(depth, p, width * sizeof(GLfloat));
         p += width;
      }
      else {
         _swrast_read_depth_span_float(ctx, readRb, width, srcx, sy, depth);
      }

      /* apply scale and bias */
      for (i = 0; i < width; i++) {
         GLfloat d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         span.array->z[i] = (GLdepth) (CLAMP(d, 0.0F, 1.0F) * depthMax);
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
   struct gl_renderbuffer *rb
      = ctx->ReadBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
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

   if (!rb) {
      /* no readbuffer - OK */
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
         MEMCPY(stencil, p, width * sizeof(GLstencil));
         p += width;
      }
      else {
         _swrast_read_stencil_span( ctx, rb, width, srcx, sy, stencil );
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
