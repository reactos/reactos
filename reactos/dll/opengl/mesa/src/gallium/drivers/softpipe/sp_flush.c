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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "draw/draw_context.h"
#include "sp_flush.h"
#include "sp_context.h"
#include "sp_state.h"
#include "sp_tile_cache.h"
#include "sp_tex_tile_cache.h"


void
softpipe_flush( struct pipe_context *pipe,
                unsigned flags,
                struct pipe_fence_handle **fence )
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   draw_flush(softpipe->draw);

   if (flags & SP_FLUSH_TEXTURE_CACHE) {
      for (i = 0; i < softpipe->num_fragment_sampler_views; i++) {
         sp_flush_tex_tile_cache(softpipe->fragment_tex_cache[i]);
      }
      for (i = 0; i < softpipe->num_vertex_sampler_views; i++) {
         sp_flush_tex_tile_cache(softpipe->vertex_tex_cache[i]);
      }
      for (i = 0; i < softpipe->num_geometry_sampler_views; i++) {
         sp_flush_tex_tile_cache(softpipe->geometry_tex_cache[i]);
      }
   }

   /* If this is a swapbuffers, just flush color buffers.
    *
    * The zbuffer changes are not discarded, but held in the cache
    * in the hope that a later clear will wipe them out.
    */
   for (i = 0; i < softpipe->framebuffer.nr_cbufs; i++)
      if (softpipe->cbuf_cache[i])
         sp_flush_tile_cache(softpipe->cbuf_cache[i]);

   if (softpipe->zsbuf_cache)
      sp_flush_tile_cache(softpipe->zsbuf_cache);

   softpipe->dirty_render_cache = FALSE;

   /* Need this call for hardware buffers before swapbuffers.
    *
    * there should probably be another/different flush-type function
    * that's called before swapbuffers because we don't always want
    * to unmap surfaces when flushing.
    */
   softpipe_unmap_transfers(softpipe);

   /* Enable to dump BMPs of the color/depth buffers each frame */
#if 0
   if(flags & PIPE_FLUSH_FRAME) {
      static unsigned frame_no = 1;
      static char filename[256];
      util_snprintf(filename, sizeof(filename), "cbuf_%u.bmp", frame_no);
      debug_dump_surface_bmp(softpipe, filename, softpipe->framebuffer.cbufs[0]);
      util_snprintf(filename, sizeof(filename), "zsbuf_%u.bmp", frame_no);
      debug_dump_surface_bmp(softpipe, filename, softpipe->framebuffer.zsbuf);
      ++frame_no;
   }
#endif

   if (fence)
      *fence = (void*)(intptr_t)1;
}

void
softpipe_flush_wrapped( struct pipe_context *pipe,
                        struct pipe_fence_handle **fence )
{
   softpipe_flush(pipe, SP_FLUSH_TEXTURE_CACHE, fence);
}


/**
 * Flush context if necessary.
 *
 * Returns FALSE if it would have block, but do_not_block was set, TRUE
 * otherwise.
 *
 * TODO: move this logic to an auxiliary library?
 */
boolean
softpipe_flush_resource(struct pipe_context *pipe,
                        struct pipe_resource *texture,
                        unsigned level,
                        int layer,
                        unsigned flush_flags,
                        boolean read_only,
                        boolean cpu_access,
                        boolean do_not_block)
{
   unsigned referenced;

   referenced = softpipe_is_resource_referenced(pipe, texture, level, layer);

   if ((referenced & SP_REFERENCED_FOR_WRITE) ||
       ((referenced & SP_REFERENCED_FOR_READ) && !read_only)) {

      /*
       * TODO: The semantics of these flush flags are too obtuse. They should
       * disappear and the pipe driver should just ensure that all visible
       * side-effects happen when they need to happen.
       */
      if (referenced & SP_REFERENCED_FOR_READ)
         flush_flags |= SP_FLUSH_TEXTURE_CACHE;

      if (cpu_access) {
         /*
          * Flush and wait.
          */

         struct pipe_fence_handle *fence = NULL;

         if (do_not_block)
            return FALSE;

         softpipe_flush(pipe, flush_flags, &fence);

         if (fence) {
            /*
             * This is for illustrative purposes only, as softpipe does not
             * have fences.
             */
            pipe->screen->fence_finish(pipe->screen, fence,
                                       PIPE_TIMEOUT_INFINITE);
            pipe->screen->fence_reference(pipe->screen, &fence, NULL);
         }
      } else {
         /*
          * Just flush.
          */

         softpipe_flush(pipe, flush_flags, NULL);
      }
   }

   return TRUE;
}
