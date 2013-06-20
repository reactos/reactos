/**************************************************************************
 *
 * Copyright 2011 Marek Olšák <maraeo@gmail.com>
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
 * IN NO EVENT SHALL AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

 /*
  * Authors:
  *   Marek Olšák <maraeo@gmail.com>
  */

#include "main/glheader.h"
#include "main/macros.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "st_context.h"
#include "st_cb_syncobj.h"

struct st_sync_object {
   struct gl_sync_object b;

   struct pipe_fence_handle *fence;
};


static struct gl_sync_object * st_new_sync_object(struct gl_context *ctx,
                                                  GLenum type)
{
   if (type == GL_SYNC_FENCE)
      return (struct gl_sync_object*)CALLOC_STRUCT(st_sync_object);
   else
      return NULL;
}

static void st_delete_sync_object(struct gl_context *ctx,
                                  struct gl_sync_object *obj)
{
   struct pipe_screen *screen = st_context(ctx)->pipe->screen;
   struct st_sync_object *so = (struct st_sync_object*)obj;

   screen->fence_reference(screen, &so->fence, NULL);
   FREE(so);
}

static void st_fence_sync(struct gl_context *ctx, struct gl_sync_object *obj,
                          GLenum condition, GLbitfield flags)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_sync_object *so = (struct st_sync_object*)obj;

   assert(condition == GL_SYNC_GPU_COMMANDS_COMPLETE && flags == 0);
   assert(so->fence == NULL);

   pipe->flush(pipe, &so->fence);
}

static void st_check_sync(struct gl_context *ctx, struct gl_sync_object *obj)
{
   struct pipe_screen *screen = st_context(ctx)->pipe->screen;
   struct st_sync_object *so = (struct st_sync_object*)obj;

   if (so->fence && screen->fence_signalled(screen, so->fence)) {
      screen->fence_reference(screen, &so->fence, NULL);
      so->b.StatusFlag = GL_TRUE;
   }
}

static void st_client_wait_sync(struct gl_context *ctx,
                                struct gl_sync_object *obj,
                                GLbitfield flags, GLuint64 timeout)
{
   struct pipe_screen *screen = st_context(ctx)->pipe->screen;
   struct st_sync_object *so = (struct st_sync_object*)obj;

   /* We don't care about GL_SYNC_FLUSH_COMMANDS_BIT, because flush is
    * already called when creating a fence. */

   if (so->fence &&
       screen->fence_finish(screen, so->fence, timeout)) {
      screen->fence_reference(screen, &so->fence, NULL);
      so->b.StatusFlag = GL_TRUE;
   }
}

static void st_server_wait_sync(struct gl_context *ctx,
                                struct gl_sync_object *obj,
                                GLbitfield flags, GLuint64 timeout)
{
   /* NO-OP.
    * Neither Gallium nor DRM interfaces support blocking on the GPU. */
}

void st_init_syncobj_functions(struct dd_function_table *functions)
{
   functions->NewSyncObject = st_new_sync_object;
   functions->FenceSync = st_fence_sync;
   functions->DeleteSyncObject = st_delete_sync_object;
   functions->CheckSync = st_check_sync;
   functions->ClientWaitSync = st_client_wait_sync;
   functions->ServerWaitSync = st_server_wait_sync;
}
