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

#include <precomp.h>

#include <main/pack.h>

/**
 * Handle a common case of drawing GL_RGB/GL_UNSIGNED_BYTE into a
 * MESA_FORMAT_XRGB888 or MESA_FORMAT_ARGB888 renderbuffer.
 */
static void
fast_draw_rgb_ubyte_pixels(struct gl_context *ctx,
                           struct gl_renderbuffer *rb,
                           GLint x, GLint y,
                           GLsizei width, GLsizei height,
                           const struct gl_pixelstore_attrib *unpack,
                           const GLvoid *pixels)
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address2d(unpack, pixels, width,
                            height, GL_RGB, GL_UNSIGNED_BYTE, 0, 0);
   const GLint srcRowStride = _mesa_image_row_stride(unpack, width,
                                                     GL_RGB, GL_UNSIGNED_BYTE);
   GLint i, j;
   GLubyte *dst;
   GLint dstRowStride;

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height,
                               GL_MAP_WRITE_BIT, &dst, &dstRowStride);

   if (!dst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDrawPixels");
      return;
   }

   if (ctx->Pixel.ZoomY == -1.0f) {
      dst = dst + (height - 1) * dstRowStride;
      dstRowStride = -dstRowStride;
   }

   for (i = 0; i < height; i++) {
      GLuint *dst4 = (GLuint *) dst;
      for (j = 0; j < width; j++) {
         dst4[j] = PACK_COLOR_8888(0xff, src[j*3+0], src[j*3+1], src[j*3+2]);
      }
      dst += dstRowStride;
      src += srcRowStride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}


/**
 * Handle a common case of drawing GL_RGBA/GL_UNSIGNED_BYTE into a
 * MESA_FORMAT_ARGB888 or MESA_FORMAT_xRGB888 renderbuffer.
 */
static void
fast_draw_rgba_ubyte_pixels(struct gl_context *ctx,
                           struct gl_renderbuffer *rb,
                           GLint x, GLint y,
                           GLsizei width, GLsizei height,
                           const struct gl_pixelstore_attrib *unpack,
                           const GLvoid *pixels)
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address2d(unpack, pixels, width,
                            height, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0);
   const GLint srcRowStride =
      _mesa_image_row_stride(unpack, width, GL_RGBA, GL_UNSIGNED_BYTE);
   GLint i, j;
   GLubyte *dst;
   GLint dstRowStride;

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height,
                               GL_MAP_WRITE_BIT, &dst, &dstRowStride);

   if (!dst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDrawPixels");
      return;
   }

   if (ctx->Pixel.ZoomY == -1.0f) {
      dst = dst + (height - 1) * dstRowStride;
      dstRowStride = -dstRowStride;
   }

   for (i = 0; i < height; i++) {
      GLuint *dst4 = (GLuint *) dst;
      for (j = 0; j < width; j++) {
         dst4[j] = PACK_COLOR_8888(src[j*4+3], src[j*4+0],
                                   src[j*4+1], src[j*4+2]);
      }
      dst += dstRowStride;
      src += srcRowStride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}


/**
 * Handle a common case of drawing a format/type combination that
 * exactly matches the renderbuffer format.
 */
static void
fast_draw_generic_pixels(struct gl_context *ctx,
                         struct gl_renderbuffer *rb,
                         GLint x, GLint y,
                         GLsizei width, GLsizei height,
                         GLenum format, GLenum type,
                         const struct gl_pixelstore_attrib *unpack,
                         const GLvoid *pixels)
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address2d(unpack, pixels, width,
                            height, format, type, 0, 0);
   const GLint srcRowStride =
      _mesa_image_row_stride(unpack, width, format, type);
   const GLint rowLength = width * _mesa_get_format_bytes(rb->Format);
   GLint i;
   GLubyte *dst;
   GLint dstRowStride;

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height,
                               GL_MAP_WRITE_BIT, &dst, &dstRowStride);

   if (!dst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDrawPixels");
      return;
   }

   if (ctx->Pixel.ZoomY == -1.0f) {
      dst = dst + (height - 1) * dstRowStride;
      dstRowStride = -dstRowStride;
   }

   for (i = 0; i < height; i++) {
      memcpy(dst, src, rowLength);
      dst += dstRowStride;
      src += srcRowStride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}


/**
 * Try to do a fast and simple RGB(a) glDrawPixels.
 * Return:  GL_TRUE if success, GL_FALSE if slow path must be used instead
 */
static GLboolean
fast_draw_rgba_pixels(struct gl_context *ctx, GLint x, GLint y,
                      GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const struct gl_pixelstore_attrib *userUnpack,
                      const GLvoid *pixels)
{
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffer;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_pixelstore_attrib unpack;

   if (!rb)
      return GL_TRUE; /* no-op */

   if ((swrast->_RasterMask & ~CLIP_BIT) ||
       ctx->Texture._EnabledCoord ||
       userUnpack->SwapBytes ||
       ctx->Pixel.ZoomX != 1.0f ||
       fabsf(ctx->Pixel.ZoomY) != 1.0f ||
       ctx->_ImageTransferState) {
      /* can't handle any of those conditions */
      return GL_FALSE;
   }

   unpack = *userUnpack;

   /* clipping */
   if (!_mesa_clip_drawpixels(ctx, &x, &y, &width, &height, &unpack)) {
      /* image was completely clipped: no-op, all done */
      return GL_TRUE;
   }

   if (format == GL_RGB &&
       type == GL_UNSIGNED_BYTE &&
       (rb->Format == MESA_FORMAT_XRGB8888 ||
        rb->Format == MESA_FORMAT_ARGB8888)) {
      fast_draw_rgb_ubyte_pixels(ctx, rb, x, y, width, height,
                                 &unpack, pixels);
      return GL_TRUE;
   }

   if (format == GL_RGBA &&
       type == GL_UNSIGNED_BYTE &&
       (rb->Format == MESA_FORMAT_XRGB8888 ||
        rb->Format == MESA_FORMAT_ARGB8888)) {
      fast_draw_rgba_ubyte_pixels(ctx, rb, x, y, width, height,
                                  &unpack, pixels);
      return GL_TRUE;
   }

   if (_mesa_format_matches_format_and_type(rb->Format, format, type)) {
      fast_draw_generic_pixels(ctx, rb, x, y, width, height,
                               format, type, &unpack, pixels);
      return GL_TRUE;
   }

   /* can't handle this pixel format and/or data type */
   return GL_FALSE;
}



/*
 * Draw stencil image.
 */
static void
draw_stencil_pixels( struct gl_context *ctx, GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum type,
                     const struct gl_pixelstore_attrib *unpack,
                     const GLvoid *pixels )
{
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0 || ctx->Pixel.ZoomY != 1.0;
   const GLenum destType = GL_UNSIGNED_BYTE;
   GLint skipPixels;

   /* if width > MAX_WIDTH, have to process image in chunks */
   skipPixels = 0;
   while (skipPixels < width) {
      const GLint spanX = x + skipPixels;
      const GLint spanWidth = MIN2(width - skipPixels, MAX_WIDTH);
      GLint row;
      for (row = 0; row < height; row++) {
         const GLint spanY = y + row;
         GLubyte values[MAX_WIDTH];
         const GLvoid *source = _mesa_image_address2d(unpack, pixels,
                                                      width, height,
                                                      GL_STENCIL_INDEX, type,
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
draw_depth_pixels( struct gl_context *ctx, GLint x, GLint y,
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
            memcpy(span.array->z, zSrc, width * sizeof(GLuint));
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
            else {
               _swrast_write_rgba_span(ctx, &span);
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
draw_rgba_pixels( struct gl_context *ctx, GLint x, GLint y,
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

   swrast_render_start(ctx);

   INIT_SPAN(span, GL_BITMAP);
   _swrast_span_default_attribs(ctx, &span);
   span.arrayMask = SPAN_RGBA;
   span.arrayAttribs = FRAG_BIT_COL; /* we're fill in COL0 attrib values */

   if (_mesa_get_format_datatype(ctx->DrawBuffer->_ColorDrawBuffer->Format) != GL_FLOAT) {
      /* need to clamp colors before applying fragment ops */
      transferOps |= IMAGE_CLAMP_BIT;
   }

   /*
    * General solution
    */
   {
      const GLbitfield interpMask = span.interpMask;
      const GLbitfield arrayMask = span.arrayMask;
      const GLint srcStride
         = _mesa_image_row_stride(unpack, width, format, type);
      GLint skipPixels = 0;
      /* use span array for temp color storage */
      GLfloat *rgba = (GLfloat *) span.array->attribs[FRAG_ATTRIB_COL];

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

            source += srcStride;
         } /* for row */

         skipPixels += spanWidth;
      } /* while skipPixels < width */

      /* XXX this is ugly/temporary, to undo above change */
      span.array->ChanType = CHAN_TYPE;
   }

   if (convImage) {
      free(convImage);
   }

   swrast_render_finish(ctx);
}

/**
 * Execute software-based glDrawPixels.
 * By time we get here, all error checking will have been done.
 */
void
_swrast_DrawPixels( struct gl_context *ctx,
		    GLint x, GLint y,
		    GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    const GLvoid *pixels )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (!pixels) {
      return;
   }

   /*
    * By time we get here, all error checking should have been done.
    */
   switch (format) {
   case GL_STENCIL_INDEX:
      swrast_render_start(ctx);
      draw_stencil_pixels( ctx, x, y, width, height, type, unpack, pixels );
      swrast_render_finish(ctx);
      break;
   case GL_DEPTH_COMPONENT:
      swrast_render_start(ctx);
      draw_depth_pixels( ctx, x, y, width, height, type, unpack, pixels );
      swrast_render_finish(ctx);
      break;
   default:
      /* all other formats should be color formats */
      draw_rgba_pixels(ctx, x, y, width, height, format, type, unpack, pixels);
   }
}
