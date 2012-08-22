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


#include "glheader.h"
#include "bufferobj.h"
#include "enums.h"
#include "context.h"
#include "formats.h"
#include "format_unpack.h"
#include "image.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "pack.h"
#include "pbo.h"
#include "texcompress.h"
#include "texgetimage.h"
#include "teximage.h"



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
   case GL_HALF_FLOAT_ARB:
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
 * glGetTexImage for depth/stencil pixels.
 */
static void
get_tex_depth_stencil(struct gl_context *ctx, GLuint dimensions,
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
            /* XXX Z24_S8 vs. S8_Z24??? */
            memcpy(dest, src, width * sizeof(GLuint));
            if (ctx->Pack.SwapBytes) {
               _mesa_swap4((GLuint *) dest, width);
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
 * Get a color texture image with decompression.
 */
static void
get_tex_rgba_compressed(struct gl_context *ctx, GLuint dimensions,
                        GLenum format, GLenum type, GLvoid *pixels,
                        struct gl_texture_image *texImage,
                        GLbitfield transferOps)
{
   /* don't want to apply sRGB -> RGB conversion here so override the format */
   const gl_format texFormat =
      _mesa_get_srgb_format_linear(texImage->TexFormat);
   const GLenum baseFormat = _mesa_get_format_base_format(texFormat);
   const GLuint width = texImage->Width;
   const GLuint height = texImage->Height;
   const GLuint depth = texImage->Depth;
   GLfloat *tempImage, *srcRow;
   GLuint row;

   /* Decompress into temp float buffer, then pack into user buffer */
   tempImage = (GLfloat *) malloc(width * height * depth
                                  * 4 * sizeof(GLfloat));
   if (!tempImage) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage()");
      return;
   }

   /* Decompress the texture image - results in 'tempImage' */
   {
      GLubyte *srcMap;
      GLint srcRowStride;
      GLuint bytes, bw, bh;

      bytes = _mesa_get_format_bytes(texFormat);
      _mesa_get_format_block_size(texFormat, &bw, &bh);

      ctx->Driver.MapTextureImage(ctx, texImage, 0,
                                  0, 0, width, height,
                                  GL_MAP_READ_BIT,
                                  &srcMap, &srcRowStride);
      if (srcMap) {
         /* XXX This line is a bit of a hack to work around the
          * mismatch of compressed row strides as returned by
          * MapTextureImage() vs. what the texture decompression code
          * uses.  This will be fixed in the future.
          */
         srcRowStride = srcRowStride * bh / bytes;

         _mesa_decompress_image(texFormat, width, height,
                                srcMap, srcRowStride, tempImage);

         ctx->Driver.UnmapTextureImage(ctx, texImage, 0);
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
      }
   }

   if (baseFormat == GL_LUMINANCE ||
       baseFormat == GL_LUMINANCE_ALPHA) {
      _mesa_rebase_rgba_float(width * height, (GLfloat (*)[4]) tempImage,
                              baseFormat);
   }

   srcRow = tempImage;
   for (row = 0; row < height; row++) {
      void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                                       width, height, format, type,
                                       0, row, 0);

      _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) srcRow,
                                 format, type, dest, &ctx->Pack, transferOps);
      srcRow += width * 4;
   }

   free(tempImage);
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
   /* don't want to apply sRGB -> RGB conversion here so override the format */
   const gl_format texFormat =
      _mesa_get_srgb_format_linear(texImage->TexFormat);
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

   if (texImage->TexObject->Target == GL_TEXTURE_1D_ARRAY) {
      depth = height;
      height = 1;
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

   if (_mesa_is_format_compressed(texImage->TexFormat)) {
      get_tex_rgba_compressed(ctx, dimensions, format, type,
                              pixels, texImage, transferOps);
   }
   else {
      get_tex_rgba_uncompressed(ctx, dimensions, format, type,
                                pixels, texImage, transferOps);
   }
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
       target == GL_TEXTURE_RECTANGLE ||
       _mesa_is_cube_face(target)) {
      if ((texImage->TexFormat == MESA_FORMAT_ARGB8888 ||
             texImage->TexFormat == MESA_FORMAT_SARGB8) &&
          format == GL_BGRA &&
          (type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_8_8_8_8_REV) &&
          !ctx->Pack.SwapBytes &&
          _mesa_little_endian()) {
         memCopy = GL_TRUE;
      }
      else if ((texImage->TexFormat == MESA_FORMAT_AL88 ||
                  texImage->TexFormat == MESA_FORMAT_SLA8) &&
               format == GL_LUMINANCE_ALPHA &&
               type == GL_UNSIGNED_BYTE &&
               !ctx->Pack.SwapBytes &&
               _mesa_little_endian()) {
         memCopy = GL_TRUE;
      }
      else if ((texImage->TexFormat == MESA_FORMAT_L8 ||
                  texImage->TexFormat == MESA_FORMAT_SL8) &&
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
   case GL_TEXTURE_3D:
      dimensions = 3;
      break;
   default:
      dimensions = 2;
   }

   /* map dest buffer, if PBO */
   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      /* Packing texture image into a PBO.
       * Map the (potentially) VRAM-based buffer into our process space so
       * we can write into it with the code below.
       * A hardware driver might use a sophisticated blit to move the
       * texture data to the PBO if the PBO is in VRAM along with the texture.
       */
      GLubyte *buf = (GLubyte *)
         ctx->Driver.MapBufferRange(ctx, 0, ctx->Pack.BufferObj->Size,
				    GL_MAP_WRITE_BIT, ctx->Pack.BufferObj);
      if (!buf) {
         /* out of memory or other unexpected error */
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage(map PBO failed)");
         return;
      }
      /* <pixels> was an offset into the PBO.
       * Now make it a real, client-side pointer inside the mapped region.
       */
      pixels = ADD_POINTERS(buf, pixels);
   }

   if (get_tex_memcpy(ctx, format, type, pixels, texImage)) {
      /* all done */
   }
   else if (format == GL_DEPTH_COMPONENT) {
      get_tex_depth(ctx, dimensions, format, type, pixels, texImage);
   }
   else if (format == GL_DEPTH_STENCIL_EXT) {
      get_tex_depth_stencil(ctx, dimensions, format, type, pixels, texImage);
   }
   else if (format == GL_YCBCR_MESA) {
      get_tex_ycbcr(ctx, dimensions, format, type, pixels, texImage);
   }
   else {
      get_tex_rgba(ctx, dimensions, format, type, pixels, texImage);
   }

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, ctx->Pack.BufferObj);
   }
}



/**
 * This is the software fallback for Driver.GetCompressedTexImage().
 * All error checking will have been done before this routine is called.
 */
void
_mesa_get_compressed_teximage(struct gl_context *ctx,
                              struct gl_texture_image *texImage,
                              GLvoid *img)
{
   const GLuint row_stride =
      _mesa_format_row_stride(texImage->TexFormat, texImage->Width);
   GLuint i;
   GLubyte *src;
   GLint srcRowStride;

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      /* pack texture image into a PBO */
      GLubyte *buf = (GLubyte *)
         ctx->Driver.MapBufferRange(ctx, 0, ctx->Pack.BufferObj->Size,
				    GL_MAP_WRITE_BIT, ctx->Pack.BufferObj);
      if (!buf) {
         /* out of memory or other unexpected error */
         _mesa_error(ctx, GL_OUT_OF_MEMORY,
                     "glGetCompresssedTexImage(map PBO failed)");
         return;
      }
      img = ADD_POINTERS(buf, img);
   }

   /* map src texture buffer */
   ctx->Driver.MapTextureImage(ctx, texImage, 0,
                               0, 0, texImage->Width, texImage->Height,
                               GL_MAP_READ_BIT, &src, &srcRowStride);

   if (src) {
      /* no pixelstore or pixel transfer, but respect stride */

      if (row_stride == srcRowStride) {
         const GLuint size = _mesa_format_image_size(texImage->TexFormat,
                                                     texImage->Width,
                                                     texImage->Height,
                                                     texImage->Depth);
         memcpy(img, src, size);
      }
      else {
         GLuint bw, bh;
         _mesa_get_format_block_size(texImage->TexFormat, &bw, &bh);
         for (i = 0; i < (texImage->Height + bh - 1) / bh; i++) {
            memcpy((GLubyte *)img + i * row_stride,
                   (GLubyte *)src + i * srcRowStride,
                   row_stride);
         }
      }

      ctx->Driver.UnmapTextureImage(ctx, texImage, 0);
   }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetCompresssedTexImage");
   }

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, ctx->Pack.BufferObj);
   }
}



/**
 * Do error checking for a glGetTexImage() call.
 * \return GL_TRUE if any error, GL_FALSE if no errors.
 */
static GLboolean
getteximage_error_check(struct gl_context *ctx, GLenum target, GLint level,
                        GLenum format, GLenum type, GLsizei clientMemSize,
                        GLvoid *pixels )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   const GLint maxLevels = _mesa_max_texture_levels(ctx, target);
   const GLuint dimensions = (target == GL_TEXTURE_3D) ? 3 : 2;
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

   if (!ctx->Extensions.ARB_depth_texture && _mesa_is_depth_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   if (!ctx->Extensions.MESA_ycbcr_texture && _mesa_is_ycbcr_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   if (!ctx->Extensions.EXT_packed_depth_stencil
       && _mesa_is_depthstencil_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   if (!ctx->Extensions.ATI_envmap_bumpmap
       && _mesa_is_dudv_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
      return GL_TRUE;
   }

   err = _mesa_error_check_format_and_type(ctx, format, type);
   if (err != GL_NO_ERROR) {
      _mesa_error(ctx, err, "glGetTexImage(format/type)");
      return GL_TRUE;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

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
            && !_mesa_is_depth_format(baseFormat)
            && !_mesa_is_depthstencil_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }
   else if (_mesa_is_ycbcr_format(format)
            && !_mesa_is_ycbcr_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }
   else if (_mesa_is_depthstencil_format(format)
            && !_mesa_is_depthstencil_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }
   else if (_mesa_is_dudv_format(format)
            && !_mesa_is_dudv_format(baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexImage(format mismatch)");
      return GL_TRUE;
   }

   if (!_mesa_validate_pbo_access(dimensions, &ctx->Pack, texImage->Width,
                                  texImage->Height, texImage->Depth,
                                  format, type, clientMemSize, pixels)) {
      if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetTexImage(out of bounds PBO access)");
      } else {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetnTexImageARB(out of bounds access:"
                     " bufSize (%d) is too small)", clientMemSize);
      }
      return GL_TRUE;
   }

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      /* PBO should not be mapped */
      if (_mesa_bufferobj_mapped(ctx->Pack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetTexImage(PBO is mapped)");
         return GL_TRUE;
      }
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
_mesa_GetnTexImageARB( GLenum target, GLint level, GLenum format,
                       GLenum type, GLsizei bufSize, GLvoid *pixels )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (getteximage_error_check(ctx, target, level, format, type,
                               bufSize, pixels)) {
      return;
   }

   if (!_mesa_is_bufferobj(ctx->Pack.BufferObj) && !pixels) {
      /* not an error, do nothing */
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);
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


void GLAPIENTRY
_mesa_GetTexImage( GLenum target, GLint level, GLenum format,
                   GLenum type, GLvoid *pixels )
{
   _mesa_GetnTexImageARB(target, level, format, type, INT_MAX, pixels);
}


/**
 * Do error checking for a glGetCompressedTexImage() call.
 * \return GL_TRUE if any error, GL_FALSE if no errors.
 */
static GLboolean
getcompressedteximage_error_check(struct gl_context *ctx, GLenum target,
                                  GLint level, GLsizei clientMemSize, GLvoid *img)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   const GLint maxLevels = _mesa_max_texture_levels(ctx, target);
   GLuint compressedSize;

   if (maxLevels == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetCompressedTexImage(target=0x%x)",
                  target);
      return GL_TRUE;
   }

   if (level < 0 || level >= maxLevels) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetCompressedTexImageARB(bad level = %d)", level);
      return GL_TRUE;
   }

   if (_mesa_is_proxy_texture(target)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetCompressedTexImageARB(bad target = %s)",
                  _mesa_lookup_enum_by_nr(target));
      return GL_TRUE;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);
   if (!texObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetCompressedTexImageARB(target)");
      return GL_TRUE;
   }

   texImage = _mesa_select_tex_image(ctx, texObj, target, level);

   if (!texImage) {
      /* probably invalid mipmap level */
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetCompressedTexImageARB(level)");
      return GL_TRUE;
   }

   if (!_mesa_is_format_compressed(texImage->TexFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetCompressedTexImageARB(texture is not compressed)");
      return GL_TRUE;
   }

   compressedSize = _mesa_format_image_size(texImage->TexFormat,
                                            texImage->Width,
                                            texImage->Height,
                                            texImage->Depth);

   if (!_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
      /* do bounds checking on writing to client memory */
      if (clientMemSize < compressedSize) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetnCompressedTexImageARB(out of bounds access:"
                     " bufSize (%d) is too small)", clientMemSize);
         return GL_TRUE;
      }
   } else {
      /* do bounds checking on PBO write */
      if ((const GLubyte *) img + compressedSize >
          (const GLubyte *) ctx->Pack.BufferObj->Size) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetCompressedTexImage(out of bounds PBO access)");
         return GL_TRUE;
      }

      /* make sure PBO is not mapped */
      if (_mesa_bufferobj_mapped(ctx->Pack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetCompressedTexImage(PBO is mapped)");
         return GL_TRUE;
      }
   }

   return GL_FALSE;
}


void GLAPIENTRY
_mesa_GetnCompressedTexImageARB(GLenum target, GLint level, GLsizei bufSize,
                                GLvoid *img)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (getcompressedteximage_error_check(ctx, target, level, bufSize, img)) {
      return;
   }

   if (!_mesa_is_bufferobj(ctx->Pack.BufferObj) && !img) {
      /* not an error, do nothing */
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);

   if (_mesa_is_zero_size_texture(texImage))
      return;

   if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE)) {
      _mesa_debug(ctx,
                  "glGetCompressedTexImage(tex %u) format = %s, w=%d, h=%d\n",
                  texObj->Name,
                  _mesa_get_format_name(texImage->TexFormat),
                  texImage->Width, texImage->Height);
   }

   _mesa_lock_texture(ctx, texObj);
   {
      ctx->Driver.GetCompressedTexImage(ctx, texImage, img);
   }
   _mesa_unlock_texture(ctx, texObj);
}

void GLAPIENTRY
_mesa_GetCompressedTexImageARB(GLenum target, GLint level, GLvoid *img)
{
   _mesa_GetnCompressedTexImageARB(target, level, INT_MAX, img);
}
