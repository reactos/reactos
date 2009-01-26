/*
 * Mesa 3-D graphics library
 * Version:  7.0.3
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
#include "main/colormac.h"
#include "main/convolve.h"
#include "main/context.h"
#include "main/feedback.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/imports.h"
#include "main/pixel.h"
#include "main/state.h"

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
   GLint i;

   if (!rb)
      return;

   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   /* process image row by row */
   for (i = 0; i < height; i++) {
      GLuint index[MAX_WIDTH];
      GLvoid *dest;
      ASSERT(rb->DataType == GL_UNSIGNED_INT);
      rb->GetRow(ctx, rb, width, x, y + i, index);

      dest = _mesa_image_address2d(packing, pixels, width, height,
                                   GL_COLOR_INDEX, type, i, 0);

      _mesa_pack_index_span(ctx, width, type, dest, index,
                            &ctx->Pack, ctx->_ImageTransferState);
   }
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
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->_DepthBuffer;
   const GLboolean biasOrScale
      = ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0;

   if (!rb)
      return;

   /* clipping should have been done already */
   ASSERT(x >= 0);
   ASSERT(y >= 0);
   ASSERT(x + width <= (GLint) rb->Width);
   ASSERT(y + height <= (GLint) rb->Height);
   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   if (type == GL_UNSIGNED_SHORT && fb->Visual.depthBits == 16
       && !biasOrScale && !packing->SwapBytes) {
      /* Special case: directly read 16-bit unsigned depth values. */
      GLint j;
      ASSERT(rb->InternalFormat == GL_DEPTH_COMPONENT16);
      ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
      for (j = 0; j < height; j++, y++) {
         void *dest =_mesa_image_address2d(packing, pixels, width, height,
                                           GL_DEPTH_COMPONENT, type, j, 0);
         rb->GetRow(ctx, rb, width, x, y, dest);
      }
   }
   else if (type == GL_UNSIGNED_INT && fb->Visual.depthBits == 24
            && !biasOrScale && !packing->SwapBytes) {
      /* Special case: directly read 24-bit unsigned depth values. */
      GLint j;
      ASSERT(rb->InternalFormat == GL_DEPTH_COMPONENT24);
      ASSERT(rb->DataType == GL_UNSIGNED_INT);
      for (j = 0; j < height; j++, y++) {
         GLuint *dest = (GLuint *)
            _mesa_image_address2d(packing, pixels, width, height,
                                  GL_DEPTH_COMPONENT, type, j, 0);
         GLint k;
         rb->GetRow(ctx, rb, width, x, y, dest);
         /* convert range from 24-bit to 32-bit */
         for (k = 0; k < width; k++) {
            /* Note: put MSByte of 24-bit value into LSByte */
            dest[k] = (dest[k] << 8) | ((dest[k] >> 16) & 0xff);
         }
      }
   }
   else if (type == GL_UNSIGNED_INT && fb->Visual.depthBits == 32
            && !biasOrScale && !packing->SwapBytes) {
      /* Special case: directly read 32-bit unsigned depth values. */
      GLint j;
      ASSERT(rb->InternalFormat == GL_DEPTH_COMPONENT32);
      ASSERT(rb->DataType == GL_UNSIGNED_INT);
      for (j = 0; j < height; j++, y++) {
         void *dest = _mesa_image_address2d(packing, pixels, width, height,
                                            GL_DEPTH_COMPONENT, type, j, 0);
         rb->GetRow(ctx, rb, width, x, y, dest);
      }
   }
   else {
      /* General case (slower) */
      GLint j;
      for (j = 0; j < height; j++, y++) {
         GLfloat depthValues[MAX_WIDTH];
         GLvoid *dest = _mesa_image_address2d(packing, pixels, width, height,
                                              GL_DEPTH_COMPONENT, type, j, 0);
         _swrast_read_depth_span_float(ctx, rb, width, x, y, depthValues);
         _mesa_pack_depth_span(ctx, width, dest, type, depthValues, packing);
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
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   GLint j;

   if (!rb)
      return;

   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   /* process image row by row */
   for (j=0;j<height;j++,y++) {
      GLvoid *dest;
      GLstencil stencil[MAX_WIDTH];

      _swrast_read_stencil_span(ctx, rb, width, x, y, stencil);

      dest = _mesa_image_address2d(packing, pixels, width, height,
                                   GL_STENCIL_INDEX, type, j, 0);

      _mesa_pack_stencil_span(ctx, width, type, dest, stencil, packing);
   }
}



/**
 * Optimized glReadPixels for particular pixel formats when pixel
 * scaling, biasing, mapping, etc. are disabled.
 * \return GL_TRUE if success, GL_FALSE if unable to do the readpixels
 */
static GLboolean
fast_read_rgba_pixels( GLcontext *ctx,
                       GLint x, GLint y,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       GLbitfield transferOps)
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;

   if (!rb)
      return GL_FALSE;

   ASSERT(rb->_BaseFormat == GL_RGBA || rb->_BaseFormat == GL_RGB);

   /* clipping should have already been done */
   ASSERT(x + width <= (GLint) rb->Width);
   ASSERT(y + height <= (GLint) rb->Height);

   /* check for things we can't handle here */
   if (transferOps ||
       packing->SwapBytes ||
       packing->LsbFirst) {
      return GL_FALSE;
   }

   if (format == GL_RGBA && rb->DataType == type) {
      const GLint dstStride = _mesa_image_row_stride(packing, width,
                                                     format, type);
      GLubyte *dest
         = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
                                             format, type, 0, 0);
      GLint row;
      ASSERT(rb->GetRow);
      for (row = 0; row < height; row++) {
         rb->GetRow(ctx, rb, width, x, y + row, dest);
         dest += dstStride;
      }
      return GL_TRUE;
   }

   if (format == GL_RGB &&
       rb->DataType == GL_UNSIGNED_BYTE &&
       type == GL_UNSIGNED_BYTE) {
      const GLint dstStride = _mesa_image_row_stride(packing, width,
                                                     format, type);
      GLubyte *dest
         = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
                                             format, type, 0, 0);
      GLint row;
      ASSERT(rb->GetRow);
      for (row = 0; row < height; row++) {
         GLubyte tempRow[MAX_WIDTH][4];
         GLint col;
         rb->GetRow(ctx, rb, width, x, y + row, tempRow);
         /* convert RGBA to RGB */
         for (col = 0; col < width; col++) {
            dest[col * 3 + 0] = tempRow[col][0];
            dest[col * 3 + 1] = tempRow[col][1];
            dest[col * 3 + 2] = tempRow[col][2];
         }
         dest += dstStride;
      }
      return GL_TRUE;
   }

   /* not handled */
   return GL_FALSE;
}


/**
 * When we're using a low-precision color buffer (like 16-bit 5/6/5)
 * we have to adjust our color values a bit to pass conformance.
 * The problem is when a 5 or 6-bit color value is convert to an 8-bit
 * value and then a floating point value, the floating point values don't
 * increment uniformly as the 5 or 6-bit value is incremented.
 *
 * This function adjusts floating point values to compensate.
 */
static void
adjust_colors(GLcontext *ctx, GLuint n, GLfloat rgba[][4])
{
   const GLuint rShift = 8 - ctx->Visual.redBits;
   const GLuint gShift = 8 - ctx->Visual.greenBits;
   const GLuint bShift = 8 - ctx->Visual.blueBits;
   const GLfloat rScale = 1.0F / (GLfloat) ((1 << ctx->Visual.redBits  ) - 1);
   const GLfloat gScale = 1.0F / (GLfloat) ((1 << ctx->Visual.greenBits) - 1);
   const GLfloat bScale = 1.0F / (GLfloat) ((1 << ctx->Visual.blueBits ) - 1);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLint r, g, b;
      /* convert float back to ubyte */
      CLAMPED_FLOAT_TO_UBYTE(r, rgba[i][RCOMP]);
      CLAMPED_FLOAT_TO_UBYTE(g, rgba[i][GCOMP]);
      CLAMPED_FLOAT_TO_UBYTE(b, rgba[i][BCOMP]);
      /* using only the N most significant bits of the ubyte value, convert to
       * float in [0,1].
       */
      rgba[i][RCOMP] = (GLfloat) (r >> rShift) * rScale;
      rgba[i][GCOMP] = (GLfloat) (g >> gShift) * gScale;
      rgba[i][BCOMP] = (GLfloat) (b >> bShift) * bScale;
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
   GLbitfield transferOps = ctx->_ImageTransferState;
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->_ColorReadBuffer;

   if (!rb)
      return;

   if (type == GL_FLOAT && ((ctx->Color.ClampReadColor == GL_TRUE) ||
                            (ctx->Color.ClampReadColor == GL_FIXED_ONLY_ARB &&
                             rb->DataType != GL_FLOAT)))
      transferOps |= IMAGE_CLAMP_BIT;

   /* Try optimized path first */
   if (fast_read_rgba_pixels(ctx, x, y, width, height,
                             format, type, pixels, packing, transferOps)) {
      return; /* done! */
   }

   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   if (ctx->Pixel.Convolution2DEnabled || ctx->Pixel.Separable2DEnabled) {
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
         if (fb->Visual.rgbMode) {
            _swrast_read_rgba_span(ctx, rb, width, x, y, GL_FLOAT, dest);
         }
         else {
            GLuint index[MAX_WIDTH];
            ASSERT(rb->DataType == GL_UNSIGNED_INT);
            rb->GetRow(ctx, rb, width, x, y, index);
            _mesa_apply_ci_transfer_ops(ctx,
                                        transferOps & IMAGE_SHIFT_OFFSET_BIT,
                                        width, index);
            _mesa_map_ci_to_rgba(ctx, width, index, (GLfloat (*)[4]) dest);
         }
         _mesa_apply_rgba_transfer_ops(ctx, 
                                      transferOps & IMAGE_PRE_CONVOLUTION_BITS,
                                      width, (GLfloat (*)[4]) dest);
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

      /* finish transfer ops and pack the resulting image */
      src = convImage;
      for (row = 0; row < height; row++) {
         GLvoid *dest;
         dest = _mesa_image_address2d(packing, pixels, width, height,
                                      format, type, row, 0);
         _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) src,
                                    format, type, dest, packing,
                                    transferOps & IMAGE_POST_CONVOLUTION_BITS);
         src += width * 4;
      }
      _mesa_free(convImage);
   }
   else {
      /* no convolution */
      const GLint dstStride
         = _mesa_image_row_stride(packing, width, format, type);
      GLfloat (*rgba)[4] = swrast->SpanArrays->attribs[FRAG_ATTRIB_COL0];
      GLint row;
      GLubyte *dst
         = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
                                             format, type, 0, 0);

      /* make sure we don't apply 1D convolution */
      transferOps &= ~(IMAGE_CONVOLUTION_BIT |
                       IMAGE_POST_CONVOLUTION_SCALE_BIAS);

      for (row = 0; row < height; row++, y++) {

         /* Get float rgba pixels */
         if (fb->Visual.rgbMode) {
            _swrast_read_rgba_span(ctx, rb, width, x, y, GL_FLOAT, rgba);
         }
         else {
            /* read CI and convert to RGBA */
            GLuint index[MAX_WIDTH];
            ASSERT(rb->DataType == GL_UNSIGNED_INT);
            rb->GetRow(ctx, rb, width, x, y, index);
            _mesa_apply_ci_transfer_ops(ctx,
                                        transferOps & IMAGE_SHIFT_OFFSET_BIT,
                                        width, index);
            _mesa_map_ci_to_rgba(ctx, width, index, rgba);
         }

         /* apply fudge factor for shallow color buffers */
         if (fb->Visual.redBits < 8 ||
             fb->Visual.greenBits < 8 ||
             fb->Visual.blueBits < 8) {
            adjust_colors(ctx, width, rgba);
         }

         /* pack the row of RGBA pixels into user's buffer */
         _mesa_pack_rgba_span_float(ctx, width, rgba, format, type, dst,
                                    packing, transferOps);

         dst += dstStride;
      }
   }
}


/**
 * Read combined depth/stencil values.
 * We'll have already done error checking to be sure the expected
 * depth and stencil buffers really exist.
 */
static void
read_depth_stencil_pixels(GLcontext *ctx,
                          GLint x, GLint y,
                          GLsizei width, GLsizei height,
                          GLenum type, GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing )
{
   const GLboolean scaleOrBias
      = ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0;
   const GLboolean stencilTransfer = ctx->Pixel.IndexShift
      || ctx->Pixel.IndexOffset || ctx->Pixel.MapStencilFlag;
   struct gl_renderbuffer *depthRb, *stencilRb;

   depthRb = ctx->ReadBuffer->_DepthBuffer;
   stencilRb = ctx->ReadBuffer->_StencilBuffer;

   if (!depthRb || !stencilRb)
      return;

   depthRb = ctx->ReadBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   stencilRb = ctx->ReadBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;

   if (depthRb->_BaseFormat == GL_DEPTH_STENCIL_EXT &&
       stencilRb->_BaseFormat == GL_DEPTH_STENCIL_EXT &&
       depthRb == stencilRb &&
       !scaleOrBias &&
       !stencilTransfer) {
      /* This is the ideal case.
       * Reading GL_DEPTH_STENCIL pixels from combined depth/stencil buffer.
       * Plus, no pixel transfer ops to worry about!
       */
      GLint i;
      GLint dstStride = _mesa_image_row_stride(packing, width,
                                               GL_DEPTH_STENCIL_EXT, type);
      GLubyte *dst = (GLubyte *) _mesa_image_address2d(packing, pixels,
                                                       width, height,
                                                       GL_DEPTH_STENCIL_EXT,
                                                       type, 0, 0);
      for (i = 0; i < height; i++) {
         depthRb->GetRow(ctx, depthRb, width, x, y + i, dst);
         dst += dstStride;
      }
   }
   else {
      /* Reading GL_DEPTH_STENCIL pixels from separate depth/stencil buffers,
       * or we need pixel transfer.
       */
      GLint i;
      depthRb = ctx->ReadBuffer->_DepthBuffer;
      stencilRb = ctx->ReadBuffer->_StencilBuffer;

      for (i = 0; i < height; i++) {
         GLstencil stencilVals[MAX_WIDTH];

         GLuint *depthStencilDst = (GLuint *)
            _mesa_image_address2d(packing, pixels, width, height,
                                  GL_DEPTH_STENCIL_EXT, type, i, 0);

         _swrast_read_stencil_span(ctx, stencilRb, width,
                                   x, y + i, stencilVals);

         if (!scaleOrBias && !stencilTransfer
             && ctx->ReadBuffer->Visual.depthBits == 24) {
            /* ideal case */
            GLuint zVals[MAX_WIDTH]; /* 24-bit values! */
            GLint j;
            ASSERT(depthRb->DataType == GL_UNSIGNED_INT);
            /* note, we've already been clipped */
            depthRb->GetRow(ctx, depthRb, width, x, y + i, zVals);
            for (j = 0; j < width; j++) {
               depthStencilDst[j] = (zVals[j] << 8) | (stencilVals[j] & 0xff);
            }
         }
         else {
            /* general case */
            GLfloat depthVals[MAX_WIDTH];
            _swrast_read_depth_span_float(ctx, depthRb, width, x, y + i,
                                          depthVals);
            _mesa_pack_depth_stencil_span(ctx, width, depthStencilDst,
                                          depthVals, stencilVals, packing);
         }
      }
   }
}



/**
 * Software fallback routine for ctx->Driver.ReadPixels().
 * By time we get here, all error checking will have been done.
 */
void
_swrast_ReadPixels( GLcontext *ctx,
		    GLint x, GLint y, GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *packing,
		    GLvoid *pixels )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_pixelstore_attrib clippedPacking = *packing;

   /* Need to do RENDER_START before clipping or anything else since this
    * is where a driver may grab the hw lock and get an updated window
    * size.
    */
   RENDER_START(swrast, ctx);

   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   /* Do all needed clipping here, so that we can forget about it later */
   if (!_mesa_clip_readpixels(ctx, &x, &y, &width, &height, &clippedPacking)) {
      /* The ReadPixels region is totally outside the window bounds */
      RENDER_FINISH(swrast, ctx);
      return;
   }

   pixels = _mesa_map_readpix_pbo(ctx, &clippedPacking, pixels);
   if (!pixels)
      return;
  
   switch (format) {
      case GL_COLOR_INDEX:
         read_index_pixels(ctx, x, y, width, height, type, pixels,
                           &clippedPacking);
	 break;
      case GL_STENCIL_INDEX:
	 read_stencil_pixels(ctx, x, y, width, height, type, pixels,
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
      case GL_DEPTH_STENCIL_EXT:
         read_depth_stencil_pixels(ctx, x, y, width, height,
                                   type, pixels, &clippedPacking);
         break;
      default:
	 _mesa_problem(ctx, "unexpected format in _swrast_ReadPixels");
         /* don't return yet, clean-up */
   }

   RENDER_FINISH(swrast, ctx);

   _mesa_unmap_readpix_pbo(ctx, &clippedPacking);
}
