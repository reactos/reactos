/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2009 VMware, Inc.
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

/**
 * Code for glGetTexImage() and glGetCompressedTexImage().
 */

#include <precomp.h>

/**
 * Can the given type represent negative values?
 */
static inline GLboolean
type_needs_clamping(GLenum type)
{
   switch (type) {
   case GL_BYTE:
   case GL_SHORT:
   case GL_INT:
   case GL_FLOAT:
   case GL_UNSIGNED_INT_10F_11F_11F_REV:
   case GL_UNSIGNED_INT_5_9_9_9_REV:
      return GL_FALSE;
   default:
      return GL_TRUE;
   }
}


/**
 * glGetTexImage for depth/Z pixels.
 */
static void
get_tex_depth(struct gl_context *ctx, GLuint dimensions,
              GLenum format, GLenum type, GLvoid *pixels,
              struct gl_texture_image *texImage)
{
   const GLint width = texImage->Width;
   const GLint height = texImage->Height;
   const GLint depth = texImage->Depth;
   GLint img, row;
   GLfloat *depthRow = (GLfloat *) malloc(width * sizeof(GLfloat));

   if (!depthRow) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
      return;
   }

   for (img = 0; img < depth; img++) {
      GLubyte *srcMap;
      GLint srcRowStride;

      /* map src texture buffer */
      ctx->Driver.MapTextureImage(ctx, texImage, img,
                                  0, 0, width, height, GL_MAP_READ_BIT,
                                  &srcMap, &srcRowStride);

      if (srcMap) {
         for (row = 0; row < height; row++) {
            void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                                             width, height, format, type,
                                             img, row, 0);
            const GLubyte *src = srcMap + row * srcRowStride;
            _mesa_unpack_float_z_row(texImage->TexFormat, width, src, depthRow);
            _mesa_pack_depth_span(ctx, width, dest, type, depthRow, &ctx->Pack);
         }

         ctx->Driver.UnmapTextureImage(ctx, texImage, img);
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
         break;
      }
   }

   free(depthRow);
}

/**
 * glGetTexImage for YCbCr pixels.
 */
static void
get_tex_ycbcr(struct gl_context *ctx, GLuint dimensions,
              GLenum format, GLenum type, GLvoid *pixels,
              struct gl_texture_image *texImage)
{
   const GLint width = texImage->Width;
   const GLint height = texImage->Height;
   const GLint depth = texImage->Depth;
   GLint img, row;

   for (img = 0; img < depth; img++) {
      GLubyte *srcMap;
      GLint rowstride;

      /* map src texture buffer */
      ctx->Driver.MapTextureImage(ctx, texImage, img,
                                  0, 0, width, height, GL_MAP_READ_BIT,
                                  &srcMap, &rowstride);

      if (srcMap) {
         for (row = 0; row < height; row++) {
            const GLubyte *src = srcMap + row * rowstride;
            void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                                             width, height, format, type,
                                             img, row, 0);
            memcpy(dest, src, width * sizeof(GLushort));

            /* check for byte swapping */
            if ((texImage->TexFormat == MESA_FORMAT_YCBCR
                 && type == GL_UNSIGNED_SHORT_8_8_REV_MESA) ||
                (texImage->TexFormat == MESA_FORMAT_YCBCR_REV
                 && type == GL_UNSIGNED_SHORT_8_8_MESA)) {
               if (!ctx->Pack.SwapBytes)
                  _mesa_swap2((GLushort *) dest, width);
            }
            else if (ctx->Pack.SwapBytes) {
               _mesa_swap2((GLushort *) dest, width);
            }
         }

         ctx->Driver.UnmapTextureImage(ctx, texImage, img);
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
         break;
      }
   }
}

/**
 * Get an uncompressed color texture image.
 */
static void
get_tex_rgba_uncompressed(struct gl_context *ctx, GLuint dimensions,
                          GLenum format, GLenum type, GLvoid *pixels,
                          struct gl_texture_image *texImage,
                          GLbitfield transferOps)
{
   const gl_format texFormat = texImage->TexFormat;
   const GLuint width = texImage->Width;
   const GLenum destBaseFormat = _mesa_base_tex_format(ctx, format);
   GLenum rebaseFormat = GL_NONE;
   GLuint height = texImage->Height;
   GLuint depth = texImage->Depth;
   GLuint img, row;
   GLfloat (*rgba)[4];
   GLuint (*rgba_uint)[4];
   GLboolean is_integer = _mesa_is_format_integer_color(texImage->TexFormat);

   /* Allocate buffer for one row of texels */
   rgba = (GLfloat (*)[4]) malloc(4 * width * sizeof(GLfloat));
   rgba_uint = (GLuint (*)[4]) rgba;
   if (!rgba) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage()");
      return;
   }

   if (texImage->_BaseFormat == GL_LUMINANCE ||
       texImage->_BaseFormat == GL_INTENSITY ||
       texImage->_BaseFormat == GL_LUMINANCE_ALPHA) {
      /* If a luminance (or intensity) texture is read back as RGB(A), the
       * returned value should be (L,0,0,1), not (L,L,L,1).  Set rebaseFormat
       * here to get G=B=0.
       */
      rebaseFormat = texImage->_BaseFormat;
   }
   else if ((texImage->_BaseFormat == GL_RGBA ||
             texImage->_BaseFormat == GL_RGB) &&
            (destBaseFormat == GL_LUMINANCE ||
             destBaseFormat == GL_LUMINANCE_ALPHA ||
             destBaseFormat == GL_LUMINANCE_INTEGER_EXT ||
             destBaseFormat == GL_LUMINANCE_ALPHA_INTEGER_EXT)) {
      /* If we're reading back an RGB(A) texture as luminance then we need
       * to return L=tex(R).  Note, that's different from glReadPixels which
       * returns L=R+G+B.
       */
      rebaseFormat = GL_LUMINANCE_ALPHA; /* this covers GL_LUMINANCE too */
   }

   for (img = 0; img < depth; img++) {
      GLubyte *srcMap;
      GLint rowstride;

      /* map src texture buffer */
      ctx->Driver.MapTextureImage(ctx, texImage, img,
                                  0, 0, width, height, GL_MAP_READ_BIT,
                                  &srcMap, &rowstride);
      if (srcMap) {
         for (row = 0; row < height; row++) {
            const GLubyte *src = srcMap + row * rowstride;
            void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                                             width, height, format, type,
                                             img, row, 0);

	    if (is_integer) {
	       _mesa_unpack_uint_rgba_row(texFormat, width, src, rgba_uint);
               if (rebaseFormat)
                  _mesa_rebase_rgba_uint(width, rgba_uint, rebaseFormat);
	       _mesa_pack_rgba_span_int(ctx, width, rgba_uint,
					format, type, dest);
	    } else {
	       _mesa_unpack_rgba_row(texFormat, width, src, rgba);
               if (rebaseFormat)
                  _mesa_rebase_rgba_float(width, rgba, rebaseFormat);
	       _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba,
					  format, type, dest,
					  &ctx->Pack, transferOps);
	    }
	 }

         /* Unmap the src texture buffer */
         ctx->Driver.UnmapTextureImage(ctx, texImage, img);
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
         break;
      }
   }

   free(rgba);
}


/**
 * glGetTexImage for color formats (RGBA, RGB, alpha, LA, etc).
 * Compressed textures are handled here as well.
 */
static void
get_tex_rgba(struct gl_context *ctx, GLuint dimensions,
             GLenum format, GLenum type, GLvoid *pixels,
             struct gl_texture_image *texImage)
{
   const GLenum dataType = _mesa_get_format_datatype(texImage->TexFormat);
   GLbitfield transferOps = 0x0;

   /* In general, clamping does not apply to glGetTexImage, except when
    * the returned type of the image can't hold negative values.
    */
   if (type_needs_clamping(type)) {
      /* the returned image type can't have negative values */
      if (dataType == GL_FLOAT ||
          dataType == GL_SIGNED_NORMALIZED ||
          format == GL_LUMINANCE ||
          format == GL_LUMINANCE_ALPHA) {
         transferOps |= IMAGE_CLAMP_BIT;
      }
   }
   /* This applies to RGB, RGBA textures. if the format is either LUMINANCE
    * or LUMINANCE ALPHA, luminance (L) is computed as L=R+G+B .we need to
    * clamp the sum to [0,1].
    */
   else if ((format == GL_LUMINANCE ||
            format == GL_LUMINANCE_ALPHA) &&
            dataType == GL_UNSIGNED_NORMALIZED) {
      transferOps |= IMAGE_CLAMP_BIT;
   }
   get_tex_rgba_uncompressed(ctx, dimensions, format, type,
                             pixels, texImage, transferOps);
}


/**
 * Try to do glGetTexImage() with simple memcpy().
 * \return GL_TRUE if done, GL_FALSE otherwise
 */
static GLboolean
get_tex_memcpy(struct gl_context *ctx, GLenum format, GLenum type,
               GLvoid *pixels,
               struct gl_texture_image *texImage)
{
   const GLenum target = texImage->TexObject->Target;
   GLboolean memCopy = GL_FALSE;

   /*
    * Check if the src/dst formats are compatible.
    * Also note that GL's pixel transfer ops don't apply to glGetTexImage()
    * so we don't have to worry about those.
    * XXX more format combinations could be supported here.
    */
   if (target == GL_TEXTURE_1D ||
       target == GL_TEXTURE_2D ||
       _mesa_is_cube_face(target)) {
      if ((texImage->TexFormat == MESA_FORMAT_ARGB8888) &&
          format == GL_BGRA &&
          (type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_8_8_8_8_REV) &&
          !ctx->Pack.SwapBytes &&
          _mesa_little_endian()) {
         memCopy = GL_TRUE;
      }
      else if ((texImage->TexFormat == MESA_FORMAT_AL88) &&
               format == GL_LUMINANCE_ALPHA &&
               type == GL_UNSIGNED_BYTE &&
               !ctx->Pack.SwapBytes &&
               _mesa_little_endian()) {
         memCopy = GL_TRUE;
      }
      else if ((texImage->TexFormat == MESA_FORMAT_L8) &&
               format == GL_LUMINANCE &&
               type == GL_UNSIGNED_BYTE) {
         memCopy = GL_TRUE;
      }
      else if (texImage->TexFormat == MESA_FORMAT_L16 &&
               format == GL_LUMINANCE &&
               type == GL_UNSIGNED_SHORT) {
         memCopy = GL_TRUE;
      }
      else if (texImage->TexFormat == MESA_FORMAT_A8 &&
               format == GL_ALPHA &&
               type == GL_UNSIGNED_BYTE) {
         memCopy = GL_TRUE;
      }
      else if (texImage->TexFormat == MESA_FORMAT_A16 &&
               format == GL_ALPHA &&
               type == GL_UNSIGNED_SHORT) {
         memCopy = GL_TRUE;
      }
   }

   if (memCopy) {
      const GLuint bpp = _mesa_get_format_bytes(texImage->TexFormat);
      const GLuint bytesPerRow = texImage->Width * bpp;
      GLubyte *dst =
         _mesa_image_address2d(&ctx->Pack, pixels, texImage->Width,
                               texImage->Height, format, type, 0, 0);
      const GLint dstRowStride =
         _mesa_image_row_stride(&ctx->Pack, texImage->Width, format, type);
      GLubyte *src;
      GLint srcRowStride;

      /* map src texture buffer */
      ctx->Driver.MapTextureImage(ctx, texImage, 0,
                                  0, 0, texImage->Width, texImage->Height,
                                  GL_MAP_READ_BIT, &src, &srcRowStride);

      if (src) {
         if (bytesPerRow == dstRowStride && bytesPerRow == srcRowStride) {
            memcpy(dst, src, bytesPerRow * texImage->Height);
         }
         else {
            GLuint row;
            for (row = 0; row < texImage->Height; row++) {
               memcpy(dst, src, bytesPerRow);
               dst += dstRowStride;
               src += srcRowStride;
            }
         }

         /* unmap src texture buffer */
         ctx->Driver.UnmapTextureImage(ctx, texImage, 0);
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
      }
   }

   return memCopy;
}


/**
 * This is the software fallback for Driver.GetTexImage().
 * All error checking will have been done before this routine is called.
 * We'll call ctx->Driver.MapTextureImage() to access the data, then
 * unmap with ctx->Driver.UnmapTextureImage().
 */
void
_mesa_get_teximage(struct gl_context *ctx,
                   GLenum format, GLenum type, GLvoid *pixels,
                   struct gl_texture_image *texImage)
{
   GLuint dimensions;

   switch (texImage->TexObject->Target) {
   case GL_TEXTURE_1D:
      dimensions = 1;
      break;
   default:
      dimensions = 2;
   }

   if (get_tex_memcpy(ctx, format, type, pixels, texImage)) {
      /* all done */
   }
   else if (format == GL_DEPTH_COMPONENT) {
      get_tex_depth(ctx, dimensions, format, type, pixels, texImage);
   }
   else if (format == GL_YCBCR_MESA) {
      get_tex_ycbcr(ctx, dimensions, format, type, pixels, texImage);
   }
   else {
      get_tex_rgba(ctx, dimensions, format, type, pixels, texImage);
   }
}

/**
 * Do error checking for a glGetTexImage() call.
 * \return GL_TRUE if any error, GL_FALSE if no errors.
 */
static GLboolean
getteximage_error_check(struct gl_context *ctx, GLenum target, GLint level,
                        GLenum format, GLenum type, GLvoid *pixels )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   const GLint maxLevels = _mesa_max_texture_levels(ctx, target);
   GLenum baseFormat, err;

   if (maxLevels == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(target=0x%x)", target);
      return GL_TRUE;
   }

   if (level < 0 || level >= maxLevels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGetTexImage(level)" );
      return GL_TRUE;
   }

   if (_mesa_sizeof_packed_type(type) <= 0) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexImage(type)" );
      return GL_TRUE;
   }

   if (_mesa_components_in_format(format) <= 0 ||
       format == GL_STENCIL_INDEX ||
       format == GL_COLOR_INDEX) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexImage(format)" );
      return GL_TRUE;
   }

   if (_mesa_is_depth_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   if (!ctx->Extensions.MESA_ycbcr_texture && _mesa_is_ycbcr_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   err = _mesa_error_check_format_and_type(ctx, format, type);
   if (err != GL_NO_ERROR) {
      _mesa_error(ctx, err, "glGetTexImage(format/type)");
      return GL_TRUE;
   }

   texObj = _mesa_select_tex_object(ctx, target);

   if (!texObj || _mesa_is_proxy_texture(target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(target)");
      return GL_TRUE;
   }

   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!texImage) {
      /* non-existant texture image */
      return GL_TRUE;
   }

   baseFormat = _mesa_get_format_base_format(texImage->TexFormat);
      
   /* Make sure the requested image format is compatible with the
    * texture's format.
    */
   if (_mesa_is_color_format(format)
       && !_mesa_is_color_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }
   else if (_mesa_is_depth_format(format)
            && !_mesa_is_depth_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }
   else if (_mesa_is_ycbcr_format(format)
            && !_mesa_is_ycbcr_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }

   return GL_FALSE;
}



/**
 * Get texture image.  Called by glGetTexImage.
 *
 * \param target texture target.
 * \param level image level.
 * \param format pixel data format for returned image.
 * \param type pixel data type for returned image.
 * \param bufSize size of the pixels data buffer.
 * \param pixels returned pixel data.
 */
void GLAPIENTRY
_mesa_GetTexImage( GLenum target, GLint level, GLenum format,
                   GLenum type, GLvoid *pixels )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (getteximage_error_check(ctx, target, level, format, type, pixels)) {
      return;
   }

   if (!pixels) {
      /* not an error, do nothing */
      return;
   }

   texObj = _mesa_select_tex_object(ctx, target);
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);

   if (_mesa_is_zero_size_texture(texImage))
      return;

   if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE)) {
      _mesa_debug(ctx, "glGetTexImage(tex %u) format = %s, w=%d, h=%d,"
                  " dstFmt=0x%x, dstType=0x%x\n",
                  texObj->Name,
                  _mesa_get_format_name(texImage->TexFormat),
                  texImage->Width, texImage->Height,
                  format, type);
   }

   _mesa_lock_texture(ctx, texObj);
   {
      ctx->Driver.GetTexImage(ctx, format, type, pixels, texImage);
   }
   _mesa_unlock_texture(ctx, texObj);
}

