/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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

/*
 * Authors:
 *   Brian Paul
 */


/*
 * The functions in this file are mostly related to software texture fallbacks.
 * This includes texture image transfer/packing and texel fetching.
 * Hardware drivers will likely override most of this.
 */



#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "context.h"
#include "convolve.h"
#include "image.h"
#include "macros.h"
#include "mipmap.h"
#include "texformat.h"
#include "teximage.h"
#include "texstore.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_span.h"


/**
 * Read an RGBA image from the frame buffer.
 * This is used by glCopyTex[Sub]Image[12]D().
 * \param x  window source x
 * \param y  window source y
 * \param width  image width
 * \param height  image height
 * \param type  datatype for returned GL_RGBA image
 * \return pointer to image
 */
static GLvoid *
read_color_image( GLcontext *ctx, GLint x, GLint y, GLenum type,
                  GLsizei width, GLsizei height )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   const GLint pixelSize = _mesa_bytes_per_pixel(GL_RGBA, type);
   const GLint stride = width * pixelSize;
   GLint row;
   GLubyte *image, *dst;

   image = (GLubyte *) _mesa_malloc(width * height * pixelSize);
   if (!image)
      return NULL;

   RENDER_START(swrast, ctx);

   dst = image;
   for (row = 0; row < height; row++) {
      _swrast_read_rgba_span(ctx, rb, width, x, y + row, type, dst);
      dst += stride;
   }

   RENDER_FINISH(swrast, ctx);

   return image;
}


/**
 * As above, but read data from depth buffer.  Returned as GLuints.
 * \sa read_color_image
 */
static GLuint *
read_depth_image( GLcontext *ctx, GLint x, GLint y,
                  GLsizei width, GLsizei height )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_DepthBuffer;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint *image, *dst;
   GLint i;

   image = (GLuint *) _mesa_malloc(width * height * sizeof(GLuint));
   if (!image)
      return NULL;

   RENDER_START(swrast, ctx);

   dst = image;
   for (i = 0; i < height; i++) {
      _swrast_read_depth_span_uint(ctx, rb, width, x, y + i, dst);
      dst += width;
   }

   RENDER_FINISH(swrast, ctx);

   return image;
}


/**
 * As above, but read data from depth+stencil buffers.
 */
static GLuint *
read_depth_stencil_image(GLcontext *ctx, GLint x, GLint y,
                         GLsizei width, GLsizei height)
{
   struct gl_renderbuffer *depthRb = ctx->ReadBuffer->_DepthBuffer;
   struct gl_renderbuffer *stencilRb = ctx->ReadBuffer->_StencilBuffer;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint *image, *dst;
   GLint i;

   ASSERT(depthRb);
   ASSERT(stencilRb);

   image = (GLuint *) _mesa_malloc(width * height * sizeof(GLuint));
   if (!image)
      return NULL;

   RENDER_START(swrast, ctx);

   /* read from depth buffer */
   dst = image;
   if (depthRb->DataType == GL_UNSIGNED_INT) {
      for (i = 0; i < height; i++) {
         _swrast_get_row(ctx, depthRb, width, x, y + i, dst, sizeof(GLuint));
         dst += width;
      }
   }
   else {
      GLushort z16[MAX_WIDTH];
      ASSERT(depthRb->DataType == GL_UNSIGNED_SHORT);
      for (i = 0; i < height; i++) {
         GLint j;
         _swrast_get_row(ctx, depthRb, width, x, y + i, z16, sizeof(GLushort));
         /* convert GLushorts to GLuints */
         for (j = 0; j < width; j++) {
            dst[j] = z16[j];
         }
         dst += width;
      }
   }

   /* put depth values into bits 0xffffff00 */
   if (ctx->ReadBuffer->Visual.depthBits == 24) {
      GLint j;
      for (j = 0; j < width * height; j++) {
         image[j] <<= 8;
      }
   }
   else if (ctx->ReadBuffer->Visual.depthBits == 16) {
      GLint j;
      for (j = 0; j < width * height; j++) {
         image[j] = (image[j] << 16) | (image[j] & 0xff00);
      }      
   }
   else {
      /* this handles arbitrary depthBits >= 12 */
      const GLint rShift = ctx->ReadBuffer->Visual.depthBits;
      const GLint lShift = 32 - rShift;
      GLint j;
      for (j = 0; j < width * height; j++) {
         GLuint z = (image[j] << lShift);
         image[j] = z | (z >> rShift);
      }
   }

   /* read stencil values and interleave into image array */
   dst = image;
   for (i = 0; i < height; i++) {
      GLstencil stencil[MAX_WIDTH];
      GLint j;
      ASSERT(8 * sizeof(GLstencil) == stencilRb->StencilBits);
      _swrast_get_row(ctx, stencilRb, width, x, y + i,
                      stencil, sizeof(GLstencil));
      for (j = 0; j < width; j++) {
         dst[j] = (dst[j] & 0xffffff00) | (stencil[j] & 0xff);
      }
      dst += width;
   }

   RENDER_FINISH(swrast, ctx);

   return image;
}


static GLboolean
is_depth_format(GLenum format)
{
   switch (format) {
      case GL_DEPTH_COMPONENT:
      case GL_DEPTH_COMPONENT16_SGIX:
      case GL_DEPTH_COMPONENT24_SGIX:
      case GL_DEPTH_COMPONENT32_SGIX:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


static GLboolean
is_depth_stencil_format(GLenum format)
{
   switch (format) {
      case GL_DEPTH_STENCIL_EXT:
      case GL_DEPTH24_STENCIL8_EXT:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


/*
 * Fallback for Driver.CopyTexImage1D().
 */
void
_swrast_copy_teximage1d( GLcontext *ctx, GLenum target, GLint level,
                         GLenum internalFormat,
                         GLint x, GLint y, GLsizei width, GLint border )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   ASSERT(texObj);
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage1D);

   if (is_depth_format(internalFormat)) {
      /* read depth image from framebuffer */
      GLuint *image = read_depth_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D");
         return;
      }
      /* call glTexImage1D to redefine the texture */
      ctx->Driver.TexImage1D(ctx, target, level, internalFormat,
                             width, border,
                             GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, image,
                             &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else if (is_depth_stencil_format(internalFormat)) {
      /* read depth/stencil image from framebuffer */
      GLuint *image = read_depth_stencil_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D");
         return;
      }
      /* call glTexImage1D to redefine the texture */
      ctx->Driver.TexImage1D(ctx, target, level, internalFormat,
                             width, border,
                             GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT,
                             image, &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else {
      /* read RGBA image from framebuffer */
      const GLenum format = GL_RGBA;
      const GLenum type = ctx->ReadBuffer->_ColorReadBuffer->DataType;
      GLvoid *image = read_color_image(ctx, x, y, type, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D");
         return;
      }
      /* call glTexImage1D to redefine the texture */
      ctx->Driver.TexImage1D(ctx, target, level, internalFormat,
                             width, border, format, type, image,
                             &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }
}


/**
 * Fallback for Driver.CopyTexImage2D().
 *
 * We implement CopyTexImage by reading the image from the framebuffer
 * then passing it to the ctx->Driver.TexImage2D() function.
 *
 * Device drivers should try to implement direct framebuffer->texture copies.
 */
void
_swrast_copy_teximage2d( GLcontext *ctx, GLenum target, GLint level,
                         GLenum internalFormat,
                         GLint x, GLint y, GLsizei width, GLsizei height,
                         GLint border )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   ASSERT(texObj);
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage2D);

   if (is_depth_format(internalFormat)) {
      /* read depth image from framebuffer */
      GLuint *image = read_depth_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D");
         return;
      }
      /* call glTexImage2D to redefine the texture */
      ctx->Driver.TexImage2D(ctx, target, level, internalFormat,
                             width, height, border,
                             GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, image,
                             &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else if (is_depth_stencil_format(internalFormat)) {
      GLuint *image = read_depth_stencil_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D");
         return;
      }
      /* call glTexImage2D to redefine the texture */
      ctx->Driver.TexImage2D(ctx, target, level, internalFormat,
                             width, height, border,
                             GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT,
                             image, &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else {
      /* read RGBA image from framebuffer */
      const GLenum format = GL_RGBA;
      const GLenum type = ctx->ReadBuffer->_ColorReadBuffer->DataType;
      GLvoid *image = read_color_image(ctx, x, y, type, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D");
         return;
      }
      /* call glTexImage2D to redefine the texture */
      ctx->Driver.TexImage2D(ctx, target, level, internalFormat,
                             width, height, border, format, type, image,
                             &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }
}


/*
 * Fallback for Driver.CopyTexSubImage1D().
 */
void
_swrast_copy_texsubimage1d( GLcontext *ctx, GLenum target, GLint level,
                            GLint xoffset, GLint x, GLint y, GLsizei width )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   ASSERT(texObj);
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage1D);

   if (texImage->_BaseFormat == GL_DEPTH_COMPONENT) {
      /* read depth image from framebuffer */
      GLuint *image = read_depth_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage1D");
         return;
      }

      /* call glTexSubImage1D to redefine the texture */
      ctx->Driver.TexSubImage1D(ctx, target, level, xoffset, width,
                                GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, image,
                                &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else if (texImage->_BaseFormat == GL_DEPTH_STENCIL_EXT) {
      /* read depth/stencil image from framebuffer */
      GLuint *image = read_depth_stencil_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage1D");
         return;
      }
      /* call glTexImage1D to redefine the texture */
      ctx->Driver.TexSubImage1D(ctx, target, level, xoffset, width,
                                GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT,
                                image, &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else {
      /* read RGBA image from framebuffer */
      const GLenum format = GL_RGBA;
      const GLenum type = ctx->ReadBuffer->_ColorReadBuffer->DataType;
      GLvoid *image = read_color_image(ctx, x, y, type, width, 1);
      if (!image) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage1D" );
         return;
      }
      /* now call glTexSubImage1D to do the real work */
      ctx->Driver.TexSubImage1D(ctx, target, level, xoffset, width,
                                format, type, image,
                                &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }
}


/**
 * Fallback for Driver.CopyTexSubImage2D().
 *
 * Read the image from the framebuffer then hand it
 * off to ctx->Driver.TexSubImage2D().
 */
void
_swrast_copy_texsubimage2d( GLcontext *ctx,
                            GLenum target, GLint level,
                            GLint xoffset, GLint yoffset,
                            GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   ASSERT(texObj);
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage2D);

   if (texImage->_BaseFormat == GL_DEPTH_COMPONENT) {
      /* read depth image from framebuffer */
      GLuint *image = read_depth_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage2D");
         return;
      }
      /* call glTexImage2D to redefine the texture */
      ctx->Driver.TexSubImage2D(ctx, target, level,
                                xoffset, yoffset, width, height,
                                GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, image,
                                &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else if (texImage->_BaseFormat == GL_DEPTH_STENCIL_EXT) {
      /* read depth/stencil image from framebuffer */
      GLuint *image = read_depth_stencil_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage2D");
         return;
      }
      /* call glTexImage2D to redefine the texture */
      ctx->Driver.TexSubImage2D(ctx, target, level,
                                xoffset, yoffset, width, height,
                                GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT,
                                image, &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else {
      /* read RGBA image from framebuffer */
      const GLenum format = GL_RGBA;
      const GLenum type = ctx->ReadBuffer->_ColorReadBuffer->DataType;
      GLvoid *image = read_color_image(ctx, x, y, type, width, height);
      if (!image) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage2D" );
         return;
      }
      /* now call glTexSubImage2D to do the real work */
      ctx->Driver.TexSubImage2D(ctx, target, level,
                                xoffset, yoffset, width, height,
                                format, type, image,
                                &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }
}


/*
 * Fallback for Driver.CopyTexSubImage3D().
 */
void
_swrast_copy_texsubimage3d( GLcontext *ctx,
                            GLenum target, GLint level,
                            GLint xoffset, GLint yoffset, GLint zoffset,
                            GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   ASSERT(texObj);
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage3D);

   if (texImage->_BaseFormat == GL_DEPTH_COMPONENT) {
      /* read depth image from framebuffer */
      GLuint *image = read_depth_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage3D");
         return;
      }
      /* call glTexImage3D to redefine the texture */
      ctx->Driver.TexSubImage3D(ctx, target, level,
                                xoffset, yoffset, zoffset, width, height, 1,
                                GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, image,
                                &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else if (texImage->_BaseFormat == GL_DEPTH_STENCIL_EXT) {
      /* read depth/stencil image from framebuffer */
      GLuint *image = read_depth_stencil_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage3D");
         return;
      }
      /* call glTexImage3D to redefine the texture */
      ctx->Driver.TexSubImage3D(ctx, target, level,
                                xoffset, yoffset, zoffset, width, height, 1,
                                GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT,
                                image, &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else {
      /* read RGBA image from framebuffer */
      const GLenum format = GL_RGBA;
      const GLenum type = ctx->ReadBuffer->_ColorReadBuffer->DataType;
      GLvoid *image = read_color_image(ctx, x, y, type, width, height);
      if (!image) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage3D" );
         return;
      }
      /* now call glTexSubImage3D to do the real work */
      ctx->Driver.TexSubImage3D(ctx, target, level,
                                xoffset, yoffset, zoffset, width, height, 1,
                                format, type, image,
                                &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }
}
