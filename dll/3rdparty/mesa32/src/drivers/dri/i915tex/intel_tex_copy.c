/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "mtypes.h"
#include "enums.h"
#include "image.h"
#include "teximage.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_fbo.h"
#include "intel_tex.h"
#include "intel_blit.h"
#include "intel_pixel.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/**
 * Get the intel_region which is the source for any glCopyTex[Sub]Image call.
 *
 * Do the best we can using the blitter.  A future project is to use
 * the texture engine and fragment programs for these copies.
 */
static const struct intel_region *
get_teximage_source(struct intel_context *intel, GLenum internalFormat)
{
   struct intel_renderbuffer *irb;

   DBG("%s %s\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(internalFormat));

   switch (internalFormat) {
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16_ARB:
      irb = intel_get_renderbuffer(intel->ctx.ReadBuffer, BUFFER_DEPTH);
      if (irb && irb->region && irb->region->cpp == 2)
         return irb->region;
      return NULL;
   case GL_DEPTH24_STENCIL8_EXT:
   case GL_DEPTH_STENCIL_EXT:
      irb = intel_get_renderbuffer(intel->ctx.ReadBuffer, BUFFER_DEPTH);
      if (irb && irb->region && irb->region->cpp == 4)
         return irb->region;
      return NULL;
   case GL_RGBA:
   case GL_RGBA8:
      return intel_readbuf_region(intel);
   case GL_RGB:
      if (intel->intelScreen->cpp == 2)
         return intel_readbuf_region(intel);
      return NULL;
   default:
      return NULL;
   }
}


static GLboolean
do_copy_texsubimage(struct intel_context *intel,
                    struct intel_texture_image *intelImage,
                    GLenum internalFormat,
                    GLint dstx, GLint dsty,
                    GLint x, GLint y, GLsizei width, GLsizei height)
{
   GLcontext *ctx = &intel->ctx;
   const struct intel_region *src =
      get_teximage_source(intel, internalFormat);

   if (!intelImage->mt || !src) {
      DBG("%s fail %p %p\n", __FUNCTION__, intelImage->mt, src);
      return GL_FALSE;
   }

   intelFlush(ctx);
   LOCK_HARDWARE(intel);
   {
      GLuint image_offset = intel_miptree_image_offset(intelImage->mt,
                                                       intelImage->face,
                                                       intelImage->level);
      const GLint orig_x = x;
      const GLint orig_y = y;
      const struct gl_framebuffer *fb = ctx->DrawBuffer;

      if (_mesa_clip_to_region(fb->_Xmin, fb->_Ymin, fb->_Xmax, fb->_Ymax,
                               &x, &y, &width, &height)) {
         /* Update dst for clipped src.  Need to also clip the source rect.
          */
         dstx += x - orig_x;
         dsty += y - orig_y;

         if (ctx->ReadBuffer->Name == 0) {
            /* reading from a window, adjust x, y */
            __DRIdrawablePrivate *dPriv = intel->driDrawable;
            GLuint window_y;
            /* window_y = position of window on screen if y=0=bottom */
            window_y = intel->intelScreen->height - (dPriv->y + dPriv->h);
            y = window_y + y;
            x += dPriv->x;
         }
         else {
            /* reading from a FBO */
            /* invert Y */
            y = ctx->ReadBuffer->Height - y - 1;
         }


         /* A bit of fiddling to get the blitter to work with -ve
          * pitches.  But we get a nice inverted blit this way, so it's
          * worth it:
          */
         intelEmitCopyBlit(intel,
                           intelImage->mt->cpp,
                           -src->pitch,
                           src->buffer,
                           src->height * src->pitch * src->cpp,
                           intelImage->mt->pitch,
                           intelImage->mt->region->buffer,
                           image_offset,
                           x, y + height, dstx, dsty, width, height,
			   GL_COPY); /* ? */

         intel_batchbuffer_flush(intel->batch);
      }
   }


   UNLOCK_HARDWARE(intel);

#if 0
   /* GL_SGIS_generate_mipmap -- this can be accelerated now.
    * XXX Add a ctx->Driver.GenerateMipmaps() function?
    */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      intel_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
#endif

   return GL_TRUE;
}





void
intelCopyTexImage1D(GLcontext * ctx, GLenum target, GLint level,
                    GLenum internalFormat,
                    GLint x, GLint y, GLsizei width, GLint border)
{
   struct gl_texture_unit *texUnit =
      &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);

   if (border)
      goto fail;

   /* Setup or redefine the texture object, mipmap tree and texture
    * image.  Don't populate yet.  
    */
   ctx->Driver.TexImage1D(ctx, target, level, internalFormat,
                          width, border,
                          GL_RGBA, CHAN_TYPE, NULL,
                          &ctx->DefaultPacking, texObj, texImage);

   if (!do_copy_texsubimage(intel_context(ctx),
                            intel_texture_image(texImage),
                            internalFormat, 0, 0, x, y, width, 1))
      goto fail;

   return;

 fail:
   _swrast_copy_teximage1d(ctx, target, level, internalFormat, x, y,
                           width, border);
}

void
intelCopyTexImage2D(GLcontext * ctx, GLenum target, GLint level,
                    GLenum internalFormat,
                    GLint x, GLint y, GLsizei width, GLsizei height,
                    GLint border)
{
   struct gl_texture_unit *texUnit =
      &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);

   if (border)
      goto fail;

   /* Setup or redefine the texture object, mipmap tree and texture
    * image.  Don't populate yet.  
    */
   ctx->Driver.TexImage2D(ctx, target, level, internalFormat,
                          width, height, border,
                          GL_RGBA, CHAN_TYPE, NULL,
                          &ctx->DefaultPacking, texObj, texImage);


   if (!do_copy_texsubimage(intel_context(ctx),
                            intel_texture_image(texImage),
                            internalFormat, 0, 0, x, y, width, height))
      goto fail;

   return;

 fail:
   _swrast_copy_teximage2d(ctx, target, level, internalFormat, x, y,
                           width, height, border);
}


void
intelCopyTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
                       GLint xoffset, GLint x, GLint y, GLsizei width)
{
   struct gl_texture_unit *texUnit =
      &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);
   GLenum internalFormat = texImage->InternalFormat;

   /* XXX need to check <border> as in above function? */

   /* Need to check texture is compatible with source format. 
    */

   if (!do_copy_texsubimage(intel_context(ctx),
                            intel_texture_image(texImage),
                            internalFormat, xoffset, 0, x, y, width, 1)) {
      _swrast_copy_texsubimage1d(ctx, target, level, xoffset, x, y, width);
   }
}



void
intelCopyTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
                       GLint xoffset, GLint yoffset,
                       GLint x, GLint y, GLsizei width, GLsizei height)
{
   struct gl_texture_unit *texUnit =
      &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj =
      _mesa_select_tex_object(ctx, texUnit, target);
   struct gl_texture_image *texImage =
      _mesa_select_tex_image(ctx, texObj, target, level);
   GLenum internalFormat = texImage->InternalFormat;


   /* Need to check texture is compatible with source format. 
    */

   if (!do_copy_texsubimage(intel_context(ctx),
                            intel_texture_image(texImage),
                            internalFormat,
                            xoffset, yoffset, x, y, width, height)) {

      DBG("%s - fallback to swrast\n", __FUNCTION__);

      _swrast_copy_texsubimage2d(ctx, target, level,
                                 xoffset, yoffset, x, y, width, height);
   }
}
