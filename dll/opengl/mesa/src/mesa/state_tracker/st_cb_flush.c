/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "main/glheader.h"
#include "main/macros.h"
#include "main/context.h"
#include "st_context.h"
#include "st_cb_bitmap.h"
#include "st_cb_flush.h"
#include "st_cb_clear.h"
#include "st_cb_fbo.h"
#include "st_manager.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "util/u_gen_mipmap.h"
#include "util/u_blit.h"


/** Check if we have a front color buffer and if it's been drawn to. */
static INLINE GLboolean
is_front_buffer_dirty(struct st_context *st)
{
   struct gl_framebuffer *fb = st->ctx->DrawBuffer;
   struct st_renderbuffer *strb
      = st_renderbuffer(fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
   return strb && strb->defined;
}


/**
 * Tell the screen to display the front color buffer on-screen.
 */
static void
display_front_buffer(struct st_context *st)
{
   struct gl_framebuffer *fb = st->ctx->DrawBuffer;
   struct st_renderbuffer *strb
      = st_renderbuffer(fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer);

   if (strb) {
      /* Hook for copying "fake" frontbuffer if necessary:
       */
      st_manager_flush_frontbuffer(st);
   }
}


void st_flush( struct st_context *st,
               struct pipe_fence_handle **fence )
{
   FLUSH_CURRENT(st->ctx, 0);

   /* Release any vertex buffers that might potentially be accessed in
    * successive frames:
    */
   st_flush_bitmap(st);
   st_flush_clear(st);
   util_blit_flush(st->blit);
   util_gen_mipmap_flush(st->gen_mipmap);

   st->pipe->flush( st->pipe, fence );
}


/**
 * Flush, and wait for completion.
 */
void st_finish( struct st_context *st )
{
   struct pipe_fence_handle *fence = NULL;

   st_flush(st, &fence);

   if(fence) {
      st->pipe->screen->fence_finish(st->pipe->screen, fence,
                                     PIPE_TIMEOUT_INFINITE);
      st->pipe->screen->fence_reference(st->pipe->screen, &fence, NULL);
   }
}



/**
 * Called via ctx->Driver.Flush()
 */
static void st_glFlush(struct gl_context *ctx)
{
   struct st_context *st = st_context(ctx);

   /* Don't call st_finish() here.  It is not the state tracker's
    * responsibilty to inject sleeps in the hope of avoiding buffer
    * synchronization issues.  Calling finish() here will just hide
    * problems that need to be fixed elsewhere.
    */
   st_flush(st, NULL);

   if (is_front_buffer_dirty(st)) {
      display_front_buffer(st);
   }
}


/**
 * Called via ctx->Driver.Finish()
 */
static void st_glFinish(struct gl_context *ctx)
{
   struct st_context *st = st_context(ctx);

   st_finish(st);

   if (is_front_buffer_dirty(st)) {
      display_front_buffer(st);
   }
}


void st_init_flush_functions(struct dd_function_table *functions)
{
   functions->Flush = st_glFlush;
   functions->Finish = st_glFinish;

   /* Windows opengl32.dll calls glFinish prior to every swapbuffers.
    * This is unnecessary and degrades performance.  Luckily we have some
    * scope to work around this, as the externally-visible behaviour of
    * Finish() is identical to Flush() in all cases - no differences in
    * rendering or ReadPixels are visible if we opt not to wait here.
    *
    * Only set this up on windows to avoid suprise elsewhere.
    */
#ifdef PIPE_OS_WINDOWS
   functions->Finish = st_glFlush;
#endif
}
