/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * \file texcompress.c
 * Helper functions for texture compression.
 */


#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "context.h"
#include "image.h"
#include "texcompress.h"
#include "texformat.h"
#include "texstore.h"

/**
 * Get the list of supported internal compression formats.
 *
 * \param ctx GL context.
 * \param formats the resulting format list (may be NULL).
 *
 * \return number of formats.
 */
GLuint
_mesa_get_compressed_formats( GLcontext *ctx, GLint *formats )
{
   GLuint n = 0;
   if (ctx->Extensions.ARB_texture_compression) {
      if (ctx->Extensions.TDFX_texture_compression_FXT1) {
         if (formats) {
            formats[n++] = GL_COMPRESSED_RGB_FXT1_3DFX;
            formats[n++] = GL_COMPRESSED_RGBA_FXT1_3DFX;
         }
         else {
            n += 2;
         }
      }
      if (ctx->Extensions.EXT_texture_compression_s3tc) {
         if (formats) {
            formats[n++] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            /* Skip this one because it has a restriction (all transparent
             * pixels become black).  See the texture compressions spec for
             * a detailed explanation.  This is what NVIDIA does.
            formats[n++] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            */
            formats[n++] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            formats[n++] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
         }
         else {
            n += 3;
         }
      }
      if (ctx->Extensions.S3_s3tc) {
         if (formats) {
            formats[n++] = GL_RGB_S3TC;
            formats[n++] = GL_RGB4_S3TC;
            formats[n++] = GL_RGBA_S3TC;
            formats[n++] = GL_RGBA4_S3TC;
         }
         else {
            n += 4;
         }
      }
   }
   return n;
}



/**
 * Return number of bytes needed to store a texture of the given size
 * using the specified compressed format.
 * This is called via the ctx->Driver.CompressedTextureSize function,
 * unless a device driver overrides it.
 *
 * \param width texture width in texels.
 * \param height texture height in texels.
 * \param depth texture depth in texels.
 * \param format - one of the specific compressed texture formats
 *
 * \return size in bytes, or zero if bad format
 */
GLuint
_mesa_compressed_texture_size( GLcontext *ctx,
                               GLsizei width, GLsizei height, GLsizei depth,
                               GLenum format )
{
   GLuint size;

   ASSERT(depth == 1);
   (void) depth;

   switch (format) {
   case GL_COMPRESSED_RGB_FXT1_3DFX:
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      /* round up width to next multiple of 8, height to next multiple of 4 */
      width = (width + 7) & ~7;
      height = (height + 3) & ~3;
      /* 16 bytes per 8x4 tile of RGB[A] texels */
      size = width * height / 2;
      /* Textures smaller than 8x4 will effectively be made into 8x4 and
       * take 16 bytes.
       */
      if (size < 16)
         size = 16;
      return size;
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
      /* round up width, height to next multiple of 4 */
      width = (width + 3) & ~3;
      height = (height + 3) & ~3;
      /* 8 bytes per 4x4 tile of RGB[A] texels */
      size = width * height / 2;
      /* Textures smaller than 4x4 will effectively be made into 4x4 and
       * take 8 bytes.
       */
      if (size < 8)
         size = 8;
      return size;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
      /* round up width, height to next multiple of 4 */
      width = (width + 3) & ~3;
      height = (height + 3) & ~3;
      /* 16 bytes per 4x4 tile of RGBA texels */
      size = width * height; /* simple! */
      /* Textures smaller than 4x4 will effectively be made into 4x4 and
       * take 16 bytes.
       */
      if (size < 16)
         size = 16;
      return size;
   default:
      _mesa_problem(ctx, "bad texformat in compressed_texture_size");
      return 0;
   }
}


/*
 * Compute the bytes per row in a compressed texture image.
 * We use this for computing the destination address for sub-texture updates.
 * \param format  one of the specific texture compression formats
 * \param width  image width in pixels
 * \return stride, in bytes, between rows for compressed image
 */
GLint
_mesa_compressed_row_stride(GLenum format, GLsizei width)
{
   GLint stride;

   switch (format) {
   case GL_COMPRESSED_RGB_FXT1_3DFX:
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      stride = ((width + 7) / 8) * 16; /* 16 bytes per 8x4 tile */
      break;
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
      stride = ((width + 3) / 4) * 8; /* 8 bytes per 4x4 tile */
      break;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
      stride = ((width + 3) / 4) * 16; /* 16 bytes per 4x4 tile */
      break;
   default:
      return 0;
   }

   return stride;
}


/*
 * Return the address of the pixel at (col, row, img) in a
 * compressed texture image.
 * \param col, row, img - image position (3D)
 * \param format - compressed image format
 * \param width - image width
 * \param image - the image address
 * \return address of pixel at (row, col)
 */
GLubyte *
_mesa_compressed_image_address(GLint col, GLint row, GLint img,
                               GLenum format,
                               GLsizei width, const GLubyte *image)
{
   GLubyte *addr;

   (void) img;

   /* We try to spot a "complete" subtexture "above" ROW, COL;
    * this texture is given by appropriate rounding of WIDTH x ROW.
    * Then we just add the amount left (usually on the left).
    *
    * Example for X*Y microtiles (Z bytes each)
    * offset = Z * (((width + X - 1) / X) * (row / Y) + col / X);
    */

   switch (format) {
   case GL_COMPRESSED_RGB_FXT1_3DFX:
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      addr = (GLubyte *) image + 16 * (((width + 7) / 8) * (row / 4) + col / 8);
      break;
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
      addr = (GLubyte *) image + 8 * (((width + 3) / 4) * (row / 4) + col / 4);
      break;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
      addr = (GLubyte *) image + 16 * (((width + 3) / 4) * (row / 4) + col / 4);
      break;
   default:
      return NULL;
   }

   return addr;
}
