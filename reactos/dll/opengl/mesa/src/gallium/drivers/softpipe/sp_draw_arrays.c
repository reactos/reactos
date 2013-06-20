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
 *    Brian Paul
 *    Keith Whitwell
 */


#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_prim.h"

#include "sp_context.h"
#include "sp_query.h"
#include "sp_state.h"
#include "sp_texture.h"

#include "draw/draw_context.h"

/**
 * This function handles drawing indexed and non-indexed prims,
 * instanced and non-instanced drawing, with or without min/max element
 * indexes.
 * All the other drawing functions are expressed in terms of this
 * function.
 *
 * For non-indexed prims, indexBuffer should be NULL.
 * For non-instanced drawing, instanceCount should be 1.
 * When the min/max element indexes aren't known, minIndex should be 0
 * and maxIndex should be ~0.
 */
void
softpipe_draw_vbo(struct pipe_context *pipe,
                  const struct pipe_draw_info *info)
{
   struct softpipe_context *sp = softpipe_context(pipe);
   struct draw_context *draw = sp->draw;
   void *mapped_indices = NULL;
   unsigned i;

   if (!softpipe_check_render_cond(sp))
      return;

   sp->reduced_api_prim = u_reduced_prim(info->mode);

   if (sp->dirty) {
      softpipe_update_derived(sp, sp->reduced_api_prim);
   }

   softpipe_map_transfers(sp);

   /* Map vertex buffers */
   for (i = 0; i < sp->num_vertex_buffers; i++) {
      void *buf = softpipe_resource(sp->vertex_buffer[i].buffer)->data;
      draw_set_mapped_vertex_buffer(draw, i, buf);
   }

   /* Map index buffer, if present */
   if (info->indexed && sp->index_buffer.buffer)
      mapped_indices = softpipe_resource(sp->index_buffer.buffer)->data;

   draw_set_mapped_index_buffer(draw, mapped_indices);

   for (i = 0; i < sp->num_so_targets; i++) {
      void *buf = softpipe_resource(sp->so_targets[i]->target.buffer)->data;
      sp->so_targets[i]->mapping = buf;
   }

   draw_set_mapped_so_targets(draw, sp->num_so_targets,
                              sp->so_targets);

   /* draw! */
   draw_vbo(draw, info);

   /* unmap vertex/index buffers - will cause draw module to flush */
   for (i = 0; i < sp->num_vertex_buffers; i++) {
      draw_set_mapped_vertex_buffer(draw, i, NULL);
   }
   if (mapped_indices) {
      draw_set_mapped_index_buffer(draw, NULL);
   }

   draw_set_mapped_so_targets(draw, 0, NULL);

   /*
    * TODO: Flush only when a user vertex/index buffer is present
    * (or even better, modify draw module to do this
    * internally when this condition is seen?)
    */
   draw_flush(draw);

   /* Note: leave drawing surfaces mapped */
   sp->dirty_render_cache = TRUE;
}
