/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "main/mfeatures.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "st_cb_eglimage.h"
#include "st_cb_fbo.h"
#include "st_context.h"
#include "st_texture.h"
#include "st_format.h"
#include "st_manager.h"

#if FEATURE_OES_EGL_image

/**
 * Return the base format just like _mesa_base_fbo_format does.
 */
static GLenum
st_pipe_format_to_base_format(enum pipe_format format)
{
   GLenum base_format;

   if (util_format_is_depth_or_stencil(format)) {
      if (util_format_is_depth_and_stencil(format)) {
         base_format = GL_DEPTH_STENCIL;
      }
      else {
         if (format == PIPE_FORMAT_S8_UINT)
            base_format = GL_STENCIL_INDEX;
         else
            base_format = GL_DEPTH_COMPONENT;
      }
   }
   else {
      /* is this enough? */
      if (util_format_has_alpha(format))
         base_format = GL_RGBA;
      else
         base_format = GL_RGB;
   }

   return base_format;
}

static void
st_egl_image_target_renderbuffer_storage(struct gl_context *ctx,
					 struct gl_renderbuffer *rb,
					 GLeglImageOES image_handle)
{
   struct st_context *st = st_context(ctx);
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   struct pipe_surface *ps;
   unsigned usage;

   usage = PIPE_BIND_RENDER_TARGET;
   ps = st_manager_get_egl_image_surface(st, (void *) image_handle, usage);
   if (ps) {
      strb->Base.Width = ps->width;
      strb->Base.Height = ps->height;
      strb->Base.Format = st_pipe_format_to_mesa_format(ps->format);
      strb->Base._BaseFormat = st_pipe_format_to_base_format(ps->format);
      strb->Base.InternalFormat = strb->Base._BaseFormat;

      pipe_surface_reference(&strb->surface, ps);
      pipe_resource_reference(&strb->texture, ps->texture);

      pipe_surface_reference(&ps, NULL);
   }
}

static void
st_bind_surface(struct gl_context *ctx, GLenum target,
                struct gl_texture_object *texObj,
                struct gl_texture_image *texImage,
                struct pipe_surface *ps)
{
   struct st_texture_object *stObj;
   struct st_texture_image *stImage;
   GLenum internalFormat;
   gl_format texFormat;

   /* map pipe format to base format */
   if (util_format_get_component_bits(ps->format, UTIL_FORMAT_COLORSPACE_RGB, 3) > 0)
      internalFormat = GL_RGBA;
   else
      internalFormat = GL_RGB;

   stObj = st_texture_object(texObj);
   stImage = st_texture_image(texImage);

   /* switch to surface based */
   if (!stObj->surface_based) {
      _mesa_clear_texture_object(ctx, texObj);
      stObj->surface_based = GL_TRUE;
   }

   texFormat = st_pipe_format_to_mesa_format(ps->format);

   _mesa_init_teximage_fields(ctx, texImage,
                              ps->width, ps->height, 1, 0, internalFormat,
                              texFormat);

   /* FIXME create a non-default sampler view from the pipe_surface? */
   pipe_resource_reference(&stObj->pt, ps->texture);
   pipe_sampler_view_reference(&stObj->sampler_view, NULL);
   pipe_resource_reference(&stImage->pt, stObj->pt);

   stObj->width0 = ps->width;
   stObj->height0 = ps->height;
   stObj->depth0 = 1;

   _mesa_dirty_texobj(ctx, texObj, GL_TRUE);
}

static void
st_egl_image_target_texture_2d(struct gl_context *ctx, GLenum target,
			       struct gl_texture_object *texObj,
			       struct gl_texture_image *texImage,
			       GLeglImageOES image_handle)
{
   struct st_context *st = st_context(ctx);
   struct pipe_surface *ps;
   unsigned usage;

   usage = PIPE_BIND_SAMPLER_VIEW;
   ps = st_manager_get_egl_image_surface(st, (void *) image_handle, usage);
   if (ps) {
      st_bind_surface(ctx, target, texObj, texImage, ps);
      pipe_surface_reference(&ps, NULL);
   }
}

void
st_init_eglimage_functions(struct dd_function_table *functions)
{
   functions->EGLImageTargetTexture2D = st_egl_image_target_texture_2d;
   functions->EGLImageTargetRenderbufferStorage = st_egl_image_target_renderbuffer_storage;
}

#endif /* FEATURE_OES_EGL_image */
