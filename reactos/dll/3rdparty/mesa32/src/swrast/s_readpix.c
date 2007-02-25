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
#include "colormac.h"
#include "convolve.h"
#include "context.h"
#include "feedback.h"
#include "image.h"
#include "macros.h"
#include "imports.h"
#include "pixel.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_span.h"
#include "s_stencil.h"



/*
 * Read a block of color index pixels.
 */
static void
read_index_pixels( GLcontext *ctx,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type, GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   /*
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   */
   GLint i, readWidth;

   /* error checking */
   if (ctx->Visual.rgbMode) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   if (type != GL_BYTE &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_SHORT &&
       type != GL_UNSIGNED_SHORT &&
       type != GL_INT &&
       type != GL_UNSIGNED_INT &&
       type != GL_FLOAT) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glReadPixels(index type)");
      return;
   }

   _swrast_use_read_buffer(ctx);

   /* XXX: width should never be > MAX_WIDTH since we did clipping earlier */
   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   /* process image row by row */
   for (i = 0; i < height; i++) {
      GLuint index[MAX_WIDTH];
      GLvoid *dest;
      ASSERT(rb->DataType == GL_UNSIGNED_INT);
      rb->GetRow(ctx, rb, readWidth, x, y + i, index);

      dest = _mesa_image_address2d(packing, pixels, width, height,
                                   GL_COLOR_INDEX, type, i, 0);

      _mesa_pack_index_span(ctx, readWidth, type, dest, index,
                            &ctx->Pack, ctx->_ImageTransferState);
   }

   _swrast_use_draw_buffer(ctx);
}



/**
 * Read pixels for format=GL_DEPTH_COMPONENT.
 */
static void
read_depth_pixels( GLcontext *ctx,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type, GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing )
{
   struct gl_renderbuffer *rb
      = ctx->ReadBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLint readWidth;
   GLboolean bias_or_scale;

   /* Error checking */
   if (ctx->Visual.depthBits <= 0 || !rb) {
      /* No depth buffer */
      _mesa_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   if (type != GL_BYTE &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_SHORT &&
       type != GL_UNSIGNED_SHORT &&
       type != GL_INT &&
       type != GL_UNSIGNED_INT &&
       type != GL_FLOAT) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glReadPixels(depth type)");
      return;
   }

   /* XXX: width should never be > MAX_WIDTH since we did clipping earlier */
   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   bias_or_scale = ctx->Pixel.DepthBias!=0.0 || ctx->Pixel.DepthScale!=1.0;

   if (type==GL_UNSIGNED_SHORT && ctx->Visual.depthBits == 16
       && !bias_or_scale && !packing->SwapBytes) {
      /* Special case: directly read 16-bit unsigned depth values. */
      GLint j;
      for (j=0;j<height;j++,y++) {
         GLdepth depth[MAX_WIDTH];
         GLushort *dst = (GLushort*) _mesa_image_address2d(packing, pixels,
                                width, height, GL_DEPTH_COMPONENT, type, j, 0);
         GLint i;
         _swrast_read_depth_span(ctx, rb, width, x, y, depth);
         for (i = 0; i < width; i++)
            dst[i] = depth[i];
      }
   }
   else if (type==GL_UNSIGNED_INT && ctx->Visual.depthBits == 32
            && !bias_or_scale && !packing->SwapBytes) {
      /* Special case: directly read 32-bit unsigned depth values. */
      GLint j;
      for (j=0;j<height;j++,y++) {
         GLdepth *dst = (GLdepth *) _mesa_image_address2d(packing, pixels,
                                width, height, GL_DEPTH_COMPONENT, type, j, 0);
         _swrast_read_depth_span(ctx, rb, width, x, y, dst);
      }
   }
   else {
      /* General case (slower) */
      GLint j;
      for (j=0;j<height;j++,y++) {
         GLfloat depth[MAX_WIDTH];
         GLvoid *dest;

         _swrast_read_depth_span_float(ctx, rb, readWidth, x, y, depth);

         dest = _mesa_image_address2d(packing, pixels, width, height,
                                      GL_DEPTH_COMPONENT, type, j, 0);

         _mesa_pack_depth_span(ctx, readWidth, (GLdepth *) dest, type,
                               depth, packing);
      }
   }
}


/**
 * Read pixels for format=GL_STENCIL_INDEX.
 */
static void
read_stencil_pixels( GLcontext *ctx,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum type, GLvoid *pixels,
                     const struct gl_pixelstore_attrib *packing )
{
   struct gl_renderbuffer *rb
      = ctx->ReadBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLint j, readWidth;

   if (type != GL_BYTE &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_SHORT &&
       type != GL_UNSIGNED_SHORT &&
       type != GL_INT &&
       type != GL_UNSIGNED_INT &&
       type != GL_FLOAT &&
       type != GL_BITMAP) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glReadPixels(stencil type)");
      return;
   }

   if (ctx->Visual.stencilBits <= 0 || !rb) {
      /* No stencil buffer */
      _mesa_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   /* XXX: width should never be > MAX_WIDTH since we did clipping earlier */
   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   /* process image row by row */
   for (j=0;j<height;j++,y++) {
      GLvoid *dest;
      GLstencil stencil[MAX_WIDTH];

      _swrast_read_stencil_span(ctx, rb, readWidth, x, y, stencil);

      dest = _mesa_image_address2d(packing, pixels, width, height,
                                   GL_STENCIL_INDEX, type, j, 0);

      _mesa_pack_stencil_span(ctx, readWidth, type, dest, stencil, packing);
   }
}



/**
 * Optimized glReadPixels for particular pixel formats:
 *   GL_UNSIGNED_BYTE, GL_RGBA
 * when pixel scaling, biasing and mapping are disabled.
 */
static GLboolean
read_fast_rgba_pixels( GLcontext *ctx,
                       GLint x, GLint y,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   /* can't do scale, bias, mapping, etc */
   if (ctx->_ImageTransferState)
       return GL_FALSE;

   /* can't do fancy pixel packing */
   if (packing->Alignment != 1 || packing->SwapBytes || packing->LsbFirst)
      return GL_FALSE;

   {
      GLint srcX = x;
      GLint srcY = y;
      GLint readWidth = width;           /* actual width read */
      GLint readHeight = height;         /* actual height read */
      GLint skipPixels = packing->SkipPixels;
      GLint skipRows = packing->SkipRows;
      GLint rowLength;

      if (packing->RowLength > 0)
         rowLength = packing->RowLength;
      else
         rowLength = width;

      /*
       * Ready to read!
       * The window region at (destX, destY) of size (readWidth, readHeight)
       * will be read back.
       * We'll write pixel data to buffer pointed to by "pixels" but we'll
       * skip "skipRows" rows and skip "skipPixels" pixels/row.
       */
#if CHAN_BITS == 8
      if (format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
#elif CHAN_BITS == 16
      if (format == GL_RGBA && type == GL_UNSIGNED_SHORT) {
#else
      if (0) {
#endif
         GLchan *dest = (GLchan *) pixels
                      + (skipRows * rowLength + skipPixels) * 4;
         GLint row;

         if (packing->Invert) {
            /* start at top and go down */
            dest += (readHeight - 1) * rowLength * 4;
            rowLength = -rowLength;
         }

         ASSERT(rb->GetRow);
         for (row=0; row<readHeight; row++) {
            rb->GetRow(ctx, rb, readWidth, srcX, srcY, dest);
            dest += rowLength * 4;
            srcY++;
         }
         return GL_TRUE;
      }
      else {
         /* can't do this format/type combination */
         return GL_FALSE;
      }
   }
}



/*
 * Read R, G, B, A, RGB, L, or LA pixels.
 */
static void
read_rgba_pixels( GLcontext *ctx,
                  GLint x, GLint y,
                  GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels,
                  const struct gl_pixelstore_attrib *packing )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   GLint readWidth;

   if (!rb) {
      /* No readbuffer is OK with GL_EXT_framebuffer_object */
      return;
   }

   /* do error checking on pixel type, format was already checked by caller */
   switch (type) {
      case GL_UNSIGNED_BYTE:
      case GL_BYTE:
      case GL_UNSIGNED_SHORT:
      case GL_SHORT:
      case GL_UNSIGNED_INT:
      case GL_INT:
      case GL_FLOAT:
      case GL_UNSIGNED_BYTE_3_3_2:
      case GL_UNSIGNED_BYTE_2_3_3_REV:
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         /* valid pixel type */
         break;
      case GL_HALF_FLOAT_ARB:
         if (!ctx->Extensions.ARB_half_float_pixel) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
            return;
         }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
         return;
   }

   if (!_mesa_is_legal_format_and_type(ctx, format, type) ||
       format == GL_INTENSITY) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(format or type)");
      return;
   }

   _swrast_use_read_buffer(ctx);

   /* Try optimized path first */
   if (read_fast_rgba_pixels( ctx, x, y, width, height,
                              format, type, pixels, packing )) {

      _swrast_use_draw_buffer(ctx);
      return; /* done! */
   }

   /* XXX: width should never be > MAX_WIDTH since we did clipping earlier */
   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;


   if (ctx->Pixel.Convolution2DEnabled || ctx->Pixel.Separable2DEnabled) {
      const GLuint transferOps = ctx->_ImageTransferState;
      GLfloat *dest, *src, *tmpImage, *convImage;
      GLint row;

      tmpImage = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
      if (!tmpImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
         return;
      }
      convImage = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
      if (!convImage) {
         _mesa_free(tmpImage);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
         return;
      }

      /* read full RGBA, FLOAT image */
      dest = tmpImage;
      for (row = 0; row < height; row++, y++) {
         GLchan rgba[MAX_WIDTH][4];
         if (ctx->Visual.rgbMode) {
            _swrast_read_rgba_span(ctx, rb, readWidth, x, y, rgba);
         }
         else {
            GLuint index[MAX_WIDTH];
            ASSERT(rb->DataType == GL_UNSIGNED_INT);
            rb->GetRow(ctx, rb, readWidth, x, y, index);
            if (ctx->Pixel.IndexShift != 0 || ctx->Pixel.IndexOffset !=0 ) {
               _mesa_map_ci(ctx, readWidth, index);
            }
            _mesa_map_ci_to_rgba_chan(ctx, readWidth, index, rgba);
         }
         _mesa_pack_rgba_span_chan(ctx, readWidth, (const GLchan (*)[4]) rgba,
                              GL_RGBA, GL_FLOAT, dest, &ctx->DefaultPacking,
                              transferOps & IMAGE_PRE_CONVOLUTION_BITS);
         dest += width * 4;
      }

      /* do convolution */
      if (ctx->Pixel.Convolution2DEnabled) {
         _mesa_convolve_2d_image(ctx, &readWidth, &height, tmpImage, convImage);
      }
      else {
         ASSERT(ctx->Pixel.Separable2DEnabled);
         _mesa_convolve_sep_image(ctx, &readWidth, &height, tmpImage, convImage);
      }
      _mesa_free(tmpImage);

      /* finish transfer ops and pack the resulting image */
      src = convImage;
      for (row = 0; row < height; row++) {
         GLvoid *dest;
         dest = _mesa_image_address2d(packing, pixels, readWidth, height,
                                      format, type, row, 0);
         _mesa_pack_rgba_span_float(ctx, readWidth,
                                    (const GLfloat (*)[4]) src,
                                    format, type, dest, packing,
                                    transferOps & IMAGE_POST_CONVOLUTION_BITS);
         src += readWidth * 4;
      }
   }
   else {
      /* no convolution */
      GLint row;
      for (row = 0; row < height; row++, y++) {
         GLchan rgba[MAX_WIDTH][4];
         GLvoid *dst;
         if (ctx->Visual.rgbMode) {
            _swrast_read_rgba_span(ctx, rb, readWidth, x, y, rgba);
         }
         else {
            GLuint index[MAX_WIDTH];
            ASSERT(rb->DataType == GL_UNSIGNED_INT);
            rb->GetRow(ctx, rb, readWidth, x, y, index);
            if (ctx->Pixel.IndexShift != 0 || ctx->Pixel.IndexOffset != 0) {
               _mesa_map_ci(ctx, readWidth, index);
            }
            _mesa_map_ci_to_rgba_chan(ctx, readWidth, index, rgba);
         }
         dst = _mesa_image_address2d(packing, pixels, width, height,
                                     format, type, row, 0);
         if (ctx->Visual.redBits < CHAN_BITS ||
             ctx->Visual.greenBits < CHAN_BITS ||
             ctx->Visual.blueBits < CHAN_BITS) {
            /* Requantize the color values into floating point and go from
             * there.  This fixes conformance failures with 16-bit color
             * buffers, for example.
             */
            DEFMARRAY(GLfloat, rgbaf, MAX_WIDTH, 4);  /* mac 32k limitation */
            CHECKARRAY(rgbaf, return);  /* mac 32k limitation */
            _mesa_chan_to_float_span(ctx, readWidth,
                                     (CONST GLchan (*)[4]) rgba, rgbaf);
            _mesa_pack_rgba_span_float(ctx, readWidth,
                                       (CONST GLfloat (*)[4]) rgbaf,
                                       format, type, dst, packing,
                                       ctx->_ImageTransferState);
            UNDEFARRAY(rgbaf);  /* mac 32k limitation */
         }
         else {
            /* GLubytes are fine */
            _mesa_pack_rgba_span_chan(ctx, readWidth, (CONST GLchan (*)[4]) rgba,
                                 format, type, dst, packing,
                                 ctx->_ImageTransferState);
         }
      }
   }

   _swrast_use_draw_buffer(ctx);
}


/**
 * Software fallback routine for ctx->Driver.ReadPixels().
 * We wind up using the swrast->ReadSpan() routines to do the job.
 */
void
_swrast_ReadPixels( GLcontext *ctx,
		    GLint x, GLint y, GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *packing,
		    GLvoid *pixels )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_pixelstore_attrib clippedPacking;

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   /* Do all needed clipping here, so that we can forget about it later */
   clippedPacking = *packing;
   if (clippedPacking.RowLength == 0) {
      clippedPacking.RowLength = width;
   }
   if (!_mesa_clip_readpixels(ctx, &x, &y, &width, &height,
                              &clippedPacking.SkipPixels,
                              &clippedPacking.SkipRows)) {
      /* The ReadPixels region is totally outside the window bounds */
      return;
   }

   if (clippedPacking.BufferObj->Name) {
      /* pack into PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(2, &clippedPacking, width, height, 1,
                                     format, type, pixels)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              clippedPacking.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(PBO is mapped)");
         return;
      }
      pixels = ADD_POINTERS(buf, pixels);
   }

   RENDER_START(swrast, ctx);

   switch (format) {
      case GL_COLOR_INDEX:
         read_index_pixels(ctx, x, y, width, height, type, pixels,
                           &clippedPacking);
	 break;
      case GL_STENCIL_INDEX:
	 read_stencil_pixels(ctx, x,y, width,height, type, pixels,
                             &clippedPacking);
         break;
      case GL_DEPTH_COMPONENT:
	 read_depth_pixels(ctx, x, y, width, height, type, pixels,
                           &clippedPacking);
	 break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
      case GL_BGR:
      case GL_BGRA:
      case GL_ABGR_EXT:
         read_rgba_pixels(ctx, x, y, width, height,
                          format, type, pixels, &clippedPacking);
	 break;
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glReadPixels(format)" );
         /* don't return yet, clean-up */
   }

   RENDER_FINISH(swrast, ctx);

   if (clippedPacking.BufferObj->Name) {
      /* done with PBO so unmap it now */
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              clippedPacking.BufferObj);
   }
}
