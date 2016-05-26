/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

/**
 * Tries to implement glReadPixels() of GL_DEPTH_COMPONENT using memcpy of the
 * mapping.
 */
static GLboolean
fast_read_depth_pixels( struct gl_context *ctx,
			GLint x, GLint y,
			GLsizei width, GLsizei height,
			GLenum type, GLvoid *pixels,
			const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLubyte *map, *dst;
   int stride, dstStride, j;

   if (ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0)
      return GL_FALSE;

   if (packing->SwapBytes)
      return GL_FALSE;

   if (_mesa_get_format_datatype(rb->Format) != GL_UNSIGNED_NORMALIZED)
      return GL_FALSE;

   if (!((type == GL_UNSIGNED_SHORT && rb->Format == MESA_FORMAT_Z16) ||
	 type == GL_UNSIGNED_INT))
      return GL_FALSE;

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);

   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return GL_TRUE;  /* don't bother trying the slow path */
   }

   dstStride = _mesa_image_row_stride(packing, width, GL_DEPTH_COMPONENT, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   GL_DEPTH_COMPONENT, type, 0, 0);

   for (j = 0; j < height; j++) {
      if (type == GL_UNSIGNED_INT) {
	 _mesa_unpack_uint_z_row(rb->Format, width, map, (GLuint *)dst);
      } else {
	 ASSERT(type == GL_UNSIGNED_SHORT && rb->Format == MESA_FORMAT_Z16);
	 memcpy(dst, map, width * 2);
      }

      map += stride;
      dst += dstStride;
   }
   ctx->Driver.UnmapRenderbuffer(ctx, rb);

   return GL_TRUE;
}

/**
 * Read pixels for format=GL_DEPTH_COMPONENT.
 */
static void
read_depth_pixels( struct gl_context *ctx,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type, GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLint j;
   GLubyte *dst, *map;
   int dstStride, stride;

   if (!rb)
      return;

   /* clipping should have been done already */
   ASSERT(x >= 0);
   ASSERT(y >= 0);
   ASSERT(x + width <= (GLint) rb->Width);
   ASSERT(y + height <= (GLint) rb->Height);
   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   if (fast_read_depth_pixels(ctx, x, y, width, height, type, pixels, packing))
      return;

   dstStride = _mesa_image_row_stride(packing, width, GL_DEPTH_COMPONENT, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   GL_DEPTH_COMPONENT, type, 0, 0);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return;
   }

   /* General case (slower) */
   for (j = 0; j < height; j++, y++) {
      GLfloat depthValues[MAX_WIDTH];
      _mesa_unpack_float_z_row(rb->Format, width, map, depthValues);
      _mesa_pack_depth_span(ctx, width, dst, type, depthValues, packing);

      dst += dstStride;
      map += stride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}


/**
 * Read pixels for format=GL_STENCIL_INDEX.
 */
static void
read_stencil_pixels( struct gl_context *ctx,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum type, GLvoid *pixels,
                     const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLint j;
   GLubyte *map;
   GLint stride;

   if (!rb)
      return;

   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return;
   }

   /* process image row by row */
   for (j = 0; j < height; j++) {
      GLvoid *dest;
      GLubyte stencil[MAX_WIDTH];

      _mesa_unpack_ubyte_stencil_row(rb->Format, width, map, stencil);
      dest = _mesa_image_address2d(packing, pixels, width, height,
                                   GL_STENCIL_INDEX, type, j, 0);

      _mesa_pack_stencil_span(ctx, width, type, dest, stencil, packing);

      map += stride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}


/**
 * Try to do glReadPixels of RGBA data using a simple memcpy or swizzle.
 * \return GL_TRUE if successful, GL_FALSE otherwise (use the slow path)
 */
static GLboolean
fast_read_rgba_pixels_memcpy( struct gl_context *ctx,
			      GLint x, GLint y,
			      GLsizei width, GLsizei height,
			      GLenum format, GLenum type,
			      GLvoid *pixels,
			      const struct gl_pixelstore_attrib *packing,
			      GLbitfield transferOps )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   GLubyte *dst, *map;
   int dstStride, stride, j, texelBytes;
   GLboolean swizzle_rb = GL_FALSE, copy_xrgb = GL_FALSE;

   /* XXX we could check for other swizzle/special cases here as needed */
   if (rb->Format == MESA_FORMAT_RGBA8888_REV &&
       format == GL_BGRA &&
       type == GL_UNSIGNED_INT_8_8_8_8_REV) {
      swizzle_rb = GL_TRUE;
   }
   else if (rb->Format == MESA_FORMAT_XRGB8888 &&
            format == GL_BGRA &&
            type == GL_UNSIGNED_INT_8_8_8_8_REV) {
      copy_xrgb = GL_TRUE;
   }
   else if (!_mesa_format_matches_format_and_type(rb->Format, format, type))
      return GL_FALSE;

   /* check for things we can't handle here */
   if (packing->SwapBytes) {
      return GL_FALSE;
   }

   /* If the format is unsigned normalized then we can ignore clamping
    * because the values are already in the range [0,1] so it won't
    * have any effect anyway.
    */
   if (_mesa_get_format_datatype(rb->Format) == GL_UNSIGNED_NORMALIZED)
      transferOps &= ~IMAGE_CLAMP_BIT;

   if (transferOps)
      return GL_FALSE;

   dstStride = _mesa_image_row_stride(packing, width, format, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   format, type, 0, 0);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return GL_TRUE;  /* don't bother trying the slow path */
   }

   texelBytes = _mesa_get_format_bytes(rb->Format);

   if (swizzle_rb) {
      /* swap R/B */
      for (j = 0; j < height; j++) {
         int i;
         for (i = 0; i < width; i++) {
            GLuint *dst4 = (GLuint *) dst, *map4 = (GLuint *) map;
            GLuint pixel = map4[i];
            dst4[i] = (pixel & 0xff00ff00)
                   | ((pixel & 0x00ff0000) >> 16)
                   | ((pixel & 0x000000ff) << 16);
         }
         dst += dstStride;
         map += stride;
      }
   } else if (copy_xrgb) {
      /* convert xrgb -> argb */
      for (j = 0; j < height; j++) {
         GLuint *dst4 = (GLuint *) dst, *map4 = (GLuint *) map;
         int i;
         for (i = 0; i < width; i++) {
            dst4[i] = map4[i] | 0xff000000;  /* set A=0xff */
         }
         dst += dstStride;
         map += stride;
      }
   } else {
      /* just memcpy */
      for (j = 0; j < height; j++) {
         memcpy(dst, map, width * texelBytes);
         dst += dstStride;
         map += stride;
      }
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);

   return GL_TRUE;
}

static void
slow_read_rgba_pixels( struct gl_context *ctx,
		       GLint x, GLint y,
		       GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       GLvoid *pixels,
		       const struct gl_pixelstore_attrib *packing,
		       GLbitfield transferOps )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   const gl_format rbFormat = rb->Format;
   void *rgba;
   GLubyte *dst, *map;
   int dstStride, stride, j;

   dstStride = _mesa_image_row_stride(packing, width, format, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   format, type, 0, 0);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glReadPixels");
      return;
   }

   rgba = malloc(width * MAX_PIXEL_BYTES);
   if (!rgba)
      goto done;

   for (j = 0; j < height; j++) {
      if (_mesa_is_integer_format(format)) {
	 _mesa_unpack_uint_rgba_row(rbFormat, width, map, (GLuint (*)[4]) rgba);
         _mesa_rebase_rgba_uint(width, (GLuint (*)[4]) rgba,
                                rb->_BaseFormat);
	 _mesa_pack_rgba_span_int(ctx, width, (GLuint (*)[4]) rgba, format,
                                  type, dst);
      } else {
	 _mesa_unpack_rgba_row(rbFormat, width, map, (GLfloat (*)[4]) rgba);
         _mesa_rebase_rgba_float(width, (GLfloat (*)[4]) rgba,
                                 rb->_BaseFormat);
	 _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba, format,
                                    type, dst, packing, transferOps);
      }
      dst += dstStride;
      map += stride;
   }

   free(rgba);

done:
   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}

/*
 * Read R, G, B, A, RGB, L, or LA pixels.
 */
static void
read_rgba_pixels( struct gl_context *ctx,
                  GLint x, GLint y,
                  GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels,
                  const struct gl_pixelstore_attrib *packing )
{
   GLbitfield transferOps = ctx->_ImageTransferState;
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->_ColorReadBuffer;

   if (!rb)
      return;

   if (!_mesa_is_integer_format(format)) {
      transferOps |= IMAGE_CLAMP_BIT;
   }

   /* Try the optimized paths first. */
   if (fast_read_rgba_pixels_memcpy(ctx, x, y, width, height,
                                    format, type, pixels, packing,
                                    transferOps)) {
      return;
   }

   slow_read_rgba_pixels(ctx, x, y, width, height,
			 format, type, pixels, packing, transferOps);
}

/**
 * Software fallback routine for ctx->Driver.ReadPixels().
 * By time we get here, all error checking will have been done.
 */
void
_mesa_readpixels(struct gl_context *ctx,
                 GLint x, GLint y, GLsizei width, GLsizei height,
                 GLenum format, GLenum type,
                 const struct gl_pixelstore_attrib *packing,
                 GLvoid *pixels)
{
   struct gl_pixelstore_attrib clippedPacking = *packing;

   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* Do all needed clipping here, so that we can forget about it later */
   if (_mesa_clip_readpixels(ctx, &x, &y, &width, &height, &clippedPacking)) {

      if (pixels) {
         switch (format) {
         case GL_STENCIL_INDEX:
            read_stencil_pixels(ctx, x, y, width, height, type, pixels,
                                &clippedPacking);
            break;
         case GL_DEPTH_COMPONENT:
            read_depth_pixels(ctx, x, y, width, height, type, pixels,
                              &clippedPacking);
            break;
         default:
            /* all other formats should be color formats */
            read_rgba_pixels(ctx, x, y, width, height, format, type, pixels,
                             &clippedPacking);
         }
      }
   }
}


/**
 * Do error checking of the format/type parameters to glReadPixels and
 * glDrawPixels.
 * \param drawing if GL_TRUE do checking for DrawPixels, else do checking
 *                for ReadPixels.
 * \return GL_TRUE if error detected, GL_FALSE if no errors
 */
GLboolean
_mesa_error_check_format_type(struct gl_context *ctx, GLenum format,
                              GLenum type, GLboolean drawing)
{
   const char *readDraw = drawing ? "Draw" : "Read";
   const GLboolean reading = !drawing;
   GLenum err;

   /* state validation should have already been done */
   ASSERT(ctx->NewState == 0x0);

   /* basic combinations test */
   err = _mesa_error_check_format_and_type(ctx, format, type);
   if (err != GL_NO_ERROR) {
      _mesa_error(ctx, err, "gl%sPixels(format or type)", readDraw);
      return GL_TRUE;
   }

   /* additional checks */
   switch (format) {
   case GL_RG:
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
   case GL_RED_INTEGER_EXT:
   case GL_GREEN_INTEGER_EXT:
   case GL_BLUE_INTEGER_EXT:
   case GL_ALPHA_INTEGER_EXT:
   case GL_RGB_INTEGER_EXT:
   case GL_RGBA_INTEGER_EXT:
   case GL_BGR_INTEGER_EXT:
   case GL_BGRA_INTEGER_EXT:
   case GL_LUMINANCE_INTEGER_EXT:
   case GL_LUMINANCE_ALPHA_INTEGER_EXT:
      if (!drawing) {
         /* reading */
         if (!_mesa_source_buffer_exists(ctx, GL_COLOR)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glReadPixels(no color buffer)");
            return GL_TRUE;
         }
      }
      break;
   case GL_COLOR_INDEX:
      if (drawing) {
         if (ctx->PixelMaps.ItoR.Size == 0 ||
	     ctx->PixelMaps.ItoG.Size == 0 ||
	     ctx->PixelMaps.ItoB.Size == 0) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                   "glDrawPixels(drawing color index pixels into RGB buffer)");
            return GL_TRUE;
         }
      }
      else {
         /* reading */
         if (!_mesa_source_buffer_exists(ctx, GL_COLOR)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glReadPixels(no color buffer)");
            return GL_TRUE;
         }
         /* We no longer support CI-mode color buffers so trying to read
          * GL_COLOR_INDEX pixels is always an error.
          */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(color buffer is RGB)");
         return GL_TRUE;
      }
      break;
   case GL_STENCIL_INDEX:
      if ((drawing && !_mesa_dest_buffer_exists(ctx, format)) ||
          (reading && !_mesa_source_buffer_exists(ctx, format))) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "gl%sPixels(no stencil buffer)", readDraw);
         return GL_TRUE;
      }
      break;
   case GL_DEPTH_COMPONENT:
      if ((drawing && !_mesa_dest_buffer_exists(ctx, format))) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "gl%sPixels(no depth buffer)", readDraw);
         return GL_TRUE;
      }
      break;
   default:
      /* this should have been caught in _mesa_error_check_format_type() */
      _mesa_problem(ctx, "unexpected format in _mesa_%sPixels", readDraw);
      return GL_TRUE;
   }

   /* no errors */
   return GL_FALSE;
}
      


void GLAPIENTRY
_mesa_ReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
    GLenum format, GLenum type, GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glReadPixels(%d, %d, %s, %s, %p)\n",
                  width, height,
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type),
                  pixels);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE,
                   "glReadPixels(width=%d height=%d)", width, height );
      return;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (_mesa_error_check_format_type(ctx, format, type, GL_FALSE)) {
      /* found an error */
      return;
   }

   /* Check that the destination format and source buffer are both
    * integer-valued or both non-integer-valued.
    */
   if (ctx->Extensions.EXT_texture_integer && _mesa_is_color_format(format)) {
      const struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
      const GLboolean srcInteger = _mesa_is_format_integer_color(rb->Format);
      const GLboolean dstInteger = _mesa_is_integer_format(format);
      if (dstInteger != srcInteger) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(integer / non-integer format mismatch");
         return;
      }
   }

   if (!_mesa_source_buffer_exists(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(no readbuffer)");
      return;
   }

   if (width == 0 || height == 0)
      return; /* nothing to do */

   ctx->Driver.ReadPixels(ctx, x, y, width, height,
			  format, type, &ctx->Pack, pixels);
}
