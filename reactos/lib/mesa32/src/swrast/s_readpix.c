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
#include "colormac.h"
#include "convolve.h"
#include "context.h"
#include "feedback.h"
#include "image.h"
#include "macros.h"
#include "imports.h"
#include "pixel.h"

#include "s_alphabuf.h"
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
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
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

   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   /* process image row by row */
   for (i = 0; i < height; i++) {
      GLuint index[MAX_WIDTH];
      GLvoid *dest;

      (*swrast->Driver.ReadCI32Span)(ctx, readWidth, x, y + i, index);

      dest = _mesa_image_address(packing, pixels, width, height,
                                 GL_COLOR_INDEX, type, 0, i, 0);

      _mesa_pack_index_span(ctx, readWidth, type, dest, index,
                            &ctx->Pack, ctx->_ImageTransferState);
   }

   _swrast_use_draw_buffer(ctx);
}



static void
read_depth_pixels( GLcontext *ctx,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type, GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing )
{
   GLint readWidth;
   GLboolean bias_or_scale;

   /* Error checking */
   if (ctx->Visual.depthBits <= 0) {
      /* No depth buffer */
      _mesa_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

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

   bias_or_scale = ctx->Pixel.DepthBias!=0.0 || ctx->Pixel.DepthScale!=1.0;

   if (type==GL_UNSIGNED_SHORT && ctx->Visual.depthBits == 16
       && !bias_or_scale && !packing->SwapBytes) {
      /* Special case: directly read 16-bit unsigned depth values. */
      GLint j;
      for (j=0;j<height;j++,y++) {
         GLdepth depth[MAX_WIDTH];
         GLushort *dst = (GLushort*) _mesa_image_address( packing, pixels,
                         width, height, GL_DEPTH_COMPONENT, type, 0, j, 0 );
         GLint i;
         _swrast_read_depth_span(ctx, width, x, y, depth);
         for (i = 0; i < width; i++)
            dst[i] = depth[i];
      }
   }
   else if (type==GL_UNSIGNED_INT && ctx->Visual.depthBits == 32
            && !bias_or_scale && !packing->SwapBytes) {
      /* Special case: directly read 32-bit unsigned depth values. */
      GLint j;
      for (j=0;j<height;j++,y++) {
         GLdepth *dst = (GLdepth *) _mesa_image_address( packing, pixels,
                         width, height, GL_DEPTH_COMPONENT, type, 0, j, 0 );
         _swrast_read_depth_span(ctx, width, x, y, dst);
      }
   }
   else {
      /* General case (slower) */
      GLint j;
      for (j=0;j<height;j++,y++) {
         GLfloat depth[MAX_WIDTH];
         GLvoid *dest;

         _swrast_read_depth_span_float(ctx, readWidth, x, y, depth);

         dest = _mesa_image_address(packing, pixels, width, height,
                                    GL_DEPTH_COMPONENT, type, 0, j, 0);

         _mesa_pack_depth_span(ctx, readWidth, (GLdepth *) dest, type,
                               depth, &ctx->Pack);
      }
   }
}



static void
read_stencil_pixels( GLcontext *ctx,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum type, GLvoid *pixels,
                     const struct gl_pixelstore_attrib *packing )
{
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

   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

   if (ctx->Visual.stencilBits <= 0) {
      /* No stencil buffer */
      _mesa_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   /* process image row by row */
   for (j=0;j<height;j++,y++) {
      GLvoid *dest;
      GLstencil stencil[MAX_WIDTH];

      _swrast_read_stencil_span(ctx, readWidth, x, y, stencil);

      dest = _mesa_image_address(packing, pixels, width, height,
                                 GL_STENCIL_INDEX, type, 0, j, 0);

      _mesa_pack_stencil_span(ctx, readWidth, type, dest, stencil, &ctx->Pack);
   }
}



/*
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
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
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

      /* horizontal clipping */
      if (srcX < 0) {
         skipPixels -= srcX;
         readWidth += srcX;
         srcX = 0;
      }
      if (srcX + readWidth > (GLint) ctx->ReadBuffer->Width)
         readWidth -= (srcX + readWidth - (GLint) ctx->ReadBuffer->Width);
      if (readWidth <= 0)
         return GL_TRUE;

      /* vertical clipping */
      if (srcY < 0) {
         skipRows -= srcY;
         readHeight += srcY;
         srcY = 0;
      }
      if (srcY + readHeight > (GLint) ctx->ReadBuffer->Height)
         readHeight -= (srcY + readHeight - (GLint) ctx->ReadBuffer->Height);
      if (readHeight <= 0)
         return GL_TRUE;

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

         for (row=0; row<readHeight; row++) {
            (*swrast->Driver.ReadRGBASpan)(ctx, readWidth, srcX, srcY,
                                        (GLchan (*)[4]) dest);
            if (ctx->DrawBuffer->UseSoftwareAlphaBuffers) {
               _swrast_read_alpha_span(ctx, readWidth, srcX, srcY,
                                     (GLchan (*)[4]) dest);
            }
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
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLint readWidth;

   _swrast_use_read_buffer(ctx);

   /* Try optimized path first */
   if (read_fast_rgba_pixels( ctx, x, y, width, height,
                              format, type, pixels, packing )) {

      _swrast_use_draw_buffer(ctx);
      return; /* done! */
   }

   readWidth = (width > MAX_WIDTH) ? MAX_WIDTH : width;

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
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
         return;
   }

   if (!_mesa_is_legal_format_and_type(format, type) ||
       format == GL_INTENSITY) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(format or type)");
      return;
   }

   if (ctx->Pixel.Convolution2DEnabled || ctx->Pixel.Separable2DEnabled) {
      const GLuint transferOps = ctx->_ImageTransferState;
      GLfloat *dest, *src, *tmpImage, *convImage;
      GLint row;

      tmpImage = (GLfloat *) MALLOC(width * height * 4 * sizeof(GLfloat));
      if (!tmpImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
         return;
      }
      convImage = (GLfloat *) MALLOC(width * height * 4 * sizeof(GLfloat));
      if (!convImage) {
         FREE(tmpImage);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
         return;
      }

      /* read full RGBA, FLOAT image */
      dest = tmpImage;
      for (row = 0; row < height; row++, y++) {
         GLchan rgba[MAX_WIDTH][4];
         if (ctx->Visual.rgbMode) {
            _swrast_read_rgba_span(ctx, ctx->ReadBuffer, readWidth, x, y, rgba);
         }
         else {
            GLuint index[MAX_WIDTH];
            (*swrast->Driver.ReadCI32Span)(ctx, readWidth, x, y, index);
            if (ctx->Pixel.IndexShift != 0 || ctx->Pixel.IndexOffset !=0 ) {
               _mesa_map_ci(ctx, readWidth, index);
            }
            _mesa_map_ci_to_rgba_chan(ctx, readWidth, index, rgba);
         }
         _mesa_pack_rgba_span(ctx, readWidth, (const GLchan (*)[4]) rgba,
                              GL_RGBA, GL_FLOAT, dest, &_mesa_native_packing,
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
      FREE(tmpImage);

      /* finish transfer ops and pack the resulting image */
      src = convImage;
      for (row = 0; row < height; row++) {
         GLvoid *dest;
         dest = _mesa_image_address(packing, pixels, readWidth, height,
                                    format, type, 0, row, 0);
         _mesa_pack_float_rgba_span(ctx, readWidth,
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
            _swrast_read_rgba_span(ctx, ctx->ReadBuffer, readWidth, x, y, rgba);
         }
         else {
            GLuint index[MAX_WIDTH];
            (*swrast->Driver.ReadCI32Span)(ctx, readWidth, x, y, index);
            if (ctx->Pixel.IndexShift != 0 || ctx->Pixel.IndexOffset != 0) {
               _mesa_map_ci(ctx, readWidth, index);
            }
            _mesa_map_ci_to_rgba_chan(ctx, readWidth, index, rgba);
         }
         dst = _mesa_image_address(packing, pixels, width, height,
                                   format, type, 0, row, 0);
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
            _mesa_pack_float_rgba_span(ctx, readWidth,
                                       (CONST GLfloat (*)[4]) rgbaf,
                                       format, type, dst, packing,
                                       ctx->_ImageTransferState);
            UNDEFARRAY(rgbaf);  /* mac 32k limitation */
         }
         else {
            /* GLubytes are fine */
            _mesa_pack_rgba_span(ctx, readWidth, (CONST GLchan (*)[4]) rgba,
                                 format, type, dst, packing,
                                 ctx->_ImageTransferState);
         }
      }
   }

   _swrast_use_draw_buffer(ctx);
}



void
_swrast_ReadPixels( GLcontext *ctx,
		    GLint x, GLint y, GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *pack,
		    GLvoid *pixels )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   (void) pack;

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   RENDER_START(swrast,ctx);

   switch (format) {
      case GL_COLOR_INDEX:
         read_index_pixels(ctx, x, y, width, height, type, pixels, &ctx->Pack);
	 break;
      case GL_STENCIL_INDEX:
	 read_stencil_pixels(ctx, x,y, width,height, type, pixels, &ctx->Pack);
         break;
      case GL_DEPTH_COMPONENT:
	 read_depth_pixels(ctx, x, y, width, height, type, pixels, &ctx->Pack);
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
                          format, type, pixels, &ctx->Pack);
	 break;
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glReadPixels(format)" );
   }

   RENDER_FINISH(swrast,ctx);
}
