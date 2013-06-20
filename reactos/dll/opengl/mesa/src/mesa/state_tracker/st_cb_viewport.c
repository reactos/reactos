/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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

#include "main/glheader.h"
#include "st_context.h"
#include "st_cb_viewport.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_atomic.h"

/**
 * Cast wrapper to convert a struct gl_framebuffer to an st_framebuffer.
 * Return NULL if the struct gl_framebuffer is a user-created framebuffer.
 * We'll only return non-null for window system framebuffers.
 * Note that this function may fail.
 */
static INLINE struct st_framebuffer *
st_ws_framebuffer(struct gl_framebuffer *fb)
{
   /* FBO cannot be casted.  See st_new_framebuffer */
   return (struct st_framebuffer *) ((fb && !fb->Name) ? fb : NULL);
}

static void st_viewport(struct gl_context * ctx, GLint x, GLint y,
                        GLsizei width, GLsizei height)
{
   struct st_context *st = ctx->st;
   struct st_framebuffer *stdraw;
   struct st_framebuffer *stread;

   if (!st->invalidate_on_gl_viewport)
      return;

   /*
    * Normally we'd want the state tracker manager to mark the drawables
    * invalid only when needed. This will force the state tracker manager
    * to revalidate the drawable, rather than just update the context with
    * the latest cached drawable info.
    */

   stdraw = st_ws_framebuffer(st->ctx->DrawBuffer);
   stread = st_ws_framebuffer(st->ctx->ReadBuffer);

   if (stdraw && stdraw->iface)
      stdraw->iface_stamp = p_atomic_read(&stdraw->iface->stamp) - 1;
   if (stread && stread != stdraw && stread->iface)
      stread->iface_stamp = p_atomic_read(&stread->iface->stamp) - 1;
}

void st_init_viewport_functions(struct dd_function_table *functions)
{
   functions->Viewport = st_viewport;
}
