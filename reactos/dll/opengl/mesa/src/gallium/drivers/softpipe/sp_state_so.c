/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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

#include "sp_context.h"
#include "sp_state.h"
#include "sp_texture.h"

#include "util/u_format.h"
#include "util/u_memory.h"
#include "draw/draw_context.h"
#include "pipebuffer/pb_buffer.h"

static struct pipe_stream_output_target *
softpipe_create_so_target(struct pipe_context *pipe,
                          struct pipe_resource *buffer,
                          unsigned buffer_offset,
                          unsigned buffer_size)
{
   struct draw_so_target *t;

   t = CALLOC_STRUCT(draw_so_target);
   t->target.context = pipe;
   t->target.reference.count = 1;
   pipe_resource_reference(&t->target.buffer, buffer);
   t->target.buffer_offset = buffer_offset;
   t->target.buffer_size = buffer_size;
   return &t->target;
}

static void
softpipe_so_target_destroy(struct pipe_context *pipe,
                           struct pipe_stream_output_target *target)
{
   pipe_resource_reference(&target->buffer, NULL);
   FREE(target);
}

static void
softpipe_set_so_targets(struct pipe_context *pipe,
                        unsigned num_targets,
                        struct pipe_stream_output_target **targets,
                        unsigned append_bitmask)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   int i;

   for (i = 0; i < num_targets; i++) {
      pipe_so_target_reference((struct pipe_stream_output_target **)&softpipe->so_targets[i], targets[i]);
   }

   for (; i < softpipe->num_so_targets; i++) {
      pipe_so_target_reference((struct pipe_stream_output_target **)&softpipe->so_targets[i], NULL);
   }

   softpipe->num_so_targets = num_targets;
}

void
softpipe_init_streamout_funcs(struct pipe_context *pipe)
{
   pipe->create_stream_output_target = softpipe_create_so_target;
   pipe->stream_output_target_destroy = softpipe_so_target_destroy;
   pipe->set_stream_output_targets = softpipe_set_so_targets;
}

