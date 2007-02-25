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
#include "texformat.h"
#include "teximage.h"
#include "texstore.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_span.h"

/*
 * Read an RGBA image from the frame buffer.
 * This is used by glCopyTex[Sub]Image[12]D().
 * Input:  ctx - the context
 *         x, y - lower left corner
 *         width, height - size of region to read
 * Return: pointer to block of GL_RGBA, GLchan data.
 */
static GLchan *
read_color_image( GLcontext *ctx, GLint x, GLint y,
                  GLsizei width, GLsizei height )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLint stride, i;
   GLchan *image, *dst;

   image = (GLchan *) _mesa_malloc(width * height * 4 * sizeof(GLchan));
   if (!image)
      return NULL;

   /* Select buffer to read from */
   _swrast_use_read_buffer(ctx);

   RENDER_START(swrast,ctx);

   dst = image;
   stride = width * 4;
   for (i = 0; i < height; i++) {
      _swrast_read_rgba_span(ctx, ctx->ReadBuffer->_ColorReadBuffer,
                             width, x, y + i, (GLchan (*)[4]) dst);
      dst += stride;
   }

   RENDER_FINISH(swrast,ctx);

   /* Read from draw buffer (the default) */
   _swrast_use_draw_buffer(ctx);

   return image;
}


/*
 * As above, but read data from depth buffer.
 */
static GLfloat *
read_depth_image( GLcontext *ctx, GLint x, GLint y,
                  GLsizei width, GLsizei height )
{
   struct gl_renderbuffer *rb
      = ctx->ReadBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLfloat *image, *dst;
   GLint i;

   image = (GLfloat *) _mesa_malloc(width * height * sizeof(GLfloat));
   if (!image)
      return NULL;

   RENDER_START(swrast,ctx);

   dst = image;
   for (i = 0; i < height; i++) {
      _swrast_read_depth_span_float(ctx, rb, width, x, y + i, dst);
      dst += width;
   }

   RENDER_FINISH(swrast,ctx);

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
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage1D);

   if (is_depth_format(internalFormat)) {
      /* read depth image from framebuffer */
      GLfloat *image = read_depth_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D");
         return;
      }

      /* call glTexImage1D to redefine the texture */
      (*ctx->Driver.TexImage1D)(ctx, target, level, internalFormat,
                                width, border,
                                GL_DEPTH_COMPONENT, GL_FLOAT, image,
                                &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else {
      /* read RGBA image from framebuffer */
      GLchan *image = read_color_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D");
         return;
      }

      /* call glTexImage1D to redefine the texture */
      (*ctx->Driver.TexImage1D)(ctx, target, level, internalFormat,
                                width, border,
                                GL_RGBA, CHAN_TYPE, image,
                                &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target, texUnit, texObj);
   }
}


/*
 * Fallback for Driver.CopyTexImage2D().
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
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage2D);

   if (is_depth_format(internalFormat)) {
      /* read depth image from framebuffer */
      GLfloat *image = read_depth_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D");
         return;
      }

      /* call glTexImage2D to redefine the texture */
      (*ctx->Driver.TexImage2D)(ctx, target, level, internalFormat,
                                width, height, border,
                                GL_DEPTH_COMPONENT, GL_FLOAT, image,
                                &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else {
      /* read RGBA image from framebuffer */
      GLchan *image = read_color_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D");
         return;
      }

      /* call glTexImage2D to redefine the texture */
      (*ctx->Driver.TexImage2D)(ctx, target, level, internalFormat,
                                width, height, border,
                                GL_RGBA, CHAN_TYPE, image,
                                &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target, texUnit, texObj);
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
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage1D);

   if (texImage->Format == GL_DEPTH_COMPONENT) {
      /* read depth image from framebuffer */
      GLfloat *image = read_depth_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage1D");
         return;
      }

      /* call glTexSubImage1D to redefine the texture */
      (*ctx->Driver.TexSubImage1D)(ctx, target, level, xoffset, width,
                                   GL_DEPTH_COMPONENT, GL_FLOAT, image,
                                   &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else {
      /* read RGBA image from framebuffer */
      GLchan *image = read_color_image(ctx, x, y, width, 1);
      if (!image) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage1D" );
         return;
      }

      /* now call glTexSubImage1D to do the real work */
      (*ctx->Driver.TexSubImage1D)(ctx, target, level, xoffset, width,
                                   GL_RGBA, CHAN_TYPE, image,
                                   &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target, texUnit, texObj);
   }
}


/*
 * Fallback for Driver.CopyTexSubImage2D().
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
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage2D);

   if (texImage->Format == GL_DEPTH_COMPONENT) {
      /* read depth image from framebuffer */
      GLfloat *image = read_depth_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage2D");
         return;
      }

      /* call glTexImage1D to redefine the texture */
      (*ctx->Driver.TexSubImage2D)(ctx, target, level,
                                   xoffset, yoffset, width, height,
                                   GL_DEPTH_COMPONENT, GL_FLOAT, image,
                                   &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else {
      /* read RGBA image from framebuffer */
      GLchan *image = read_color_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage2D" );
         return;
      }

      /* now call glTexSubImage2D to do the real work */
      (*ctx->Driver.TexSubImage2D)(ctx, target, level,
                                   xoffset, yoffset, width, height,
                                   GL_RGBA, CHAN_TYPE, image,
                                   &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target, texUnit, texObj);
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
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   ASSERT(texImage);

   ASSERT(ctx->Driver.TexImage3D);

   if (texImage->Format == GL_DEPTH_COMPONENT) {
      /* read depth image from framebuffer */
      GLfloat *image = read_depth_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage3D");
         return;
      }

      /* call glTexImage1D to redefine the texture */
      (*ctx->Driver.TexSubImage3D)(ctx, target, level,
                                   xoffset, yoffset, zoffset, width, height, 1,
                                   GL_DEPTH_COMPONENT, GL_FLOAT, image,
                                   &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }
   else {
      /* read RGBA image from framebuffer */
      GLchan *image = read_color_image(ctx, x, y, width, height);
      if (!image) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage3D" );
         return;
      }

      /* now call glTexSubImage3D to do the real work */
      (*ctx->Driver.TexSubImage3D)(ctx, target, level,
                                   xoffset, yoffset, zoffset, width, height, 1,
                                   GL_RGBA, CHAN_TYPE, image,
                                   &ctx->DefaultPacking, texObj, texImage);
      _mesa_free(image);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target, texUnit, texObj);
   }
}
