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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */


#include "sp_context.h"
#include "sp_state.h"

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_transfer.h"
#include "draw/draw_context.h"


static void *
softpipe_create_vertex_elements_state(struct pipe_context *pipe,
                                      unsigned count,
                                      const struct pipe_vertex_element *attribs)
{
   struct sp_velems_state *velems;
   assert(count <= PIPE_MAX_ATTRIBS);
   velems = (struct sp_velems_state *) MALLOC(sizeof(struct sp_velems_state));
   if (velems) {
      velems->count = count;
      memcpy(velems->velem, attribs, sizeof(*attribs) * count);
   }
   return velems;
}


static void
softpipe_bind_vertex_elements_state(struct pipe_context *pipe,
                                    void *velems)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_velems_state *sp_velems = (struct sp_velems_state *) velems;

   softpipe->velems = sp_velems;

   softpipe->dirty |= SP_NEW_VERTEX;

   if (sp_velems)
      draw_set_vertex_elements(softpipe->draw, sp_velems->count, sp_velems->velem);
}


static void
softpipe_delete_vertex_elements_state(struct pipe_context *pipe, void *velems)
{
   FREE( velems );
}


static void
softpipe_set_vertex_buffers(struct pipe_context *pipe,
                            unsigned count,
                            const struct pipe_vertex_buffer *buffers)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   assert(count <= PIPE_MAX_ATTRIBS);

   util_copy_vertex_buffers(softpipe->vertex_buffer,
                            &softpipe->num_vertex_buffers,
                            buffers, count);

   softpipe->dirty |= SP_NEW_VERTEX;

   draw_set_vertex_buffers(softpipe->draw, count, buffers);
}


static void
softpipe_set_index_buffer(struct pipe_context *pipe,
                          const struct pipe_index_buffer *ib)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   if (ib)
      memcpy(&softpipe->index_buffer, ib, sizeof(softpipe->index_buffer));
   else
      memset(&softpipe->index_buffer, 0, sizeof(softpipe->index_buffer));

   draw_set_index_buffer(softpipe->draw, ib);
}


void
softpipe_init_vertex_funcs(struct pipe_context *pipe)
{
   pipe->create_vertex_elements_state = softpipe_create_vertex_elements_state;
   pipe->bind_vertex_elements_state = softpipe_bind_vertex_elements_state;
   pipe->delete_vertex_elements_state = softpipe_delete_vertex_elements_state;

   pipe->set_vertex_buffers = softpipe_set_vertex_buffers;
   pipe->set_index_buffer = softpipe_set_index_buffer;
   pipe->redefine_user_buffer = u_default_redefine_user_buffer;
}
