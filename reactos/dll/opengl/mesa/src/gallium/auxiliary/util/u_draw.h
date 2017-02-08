/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef U_DRAW_H
#define U_DRAW_H


#include "pipe/p_compiler.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"


#ifdef __cplusplus
extern "C" {
#endif


static INLINE void
util_draw_init_info(struct pipe_draw_info *info)
{
   memset(info, 0, sizeof(*info));
   info->instance_count = 1;
   info->max_index = 0xffffffff;
}


static INLINE void
util_draw_arrays(struct pipe_context *pipe, uint mode, uint start, uint count)
{
   struct pipe_draw_info info;

   util_draw_init_info(&info);
   info.mode = mode;
   info.start = start;
   info.count = count;
   info.min_index = start;
   info.max_index = start + count - 1;

   pipe->draw_vbo(pipe, &info);
}

static INLINE void
util_draw_elements(struct pipe_context *pipe, int index_bias,
                   uint mode, uint start, uint count)
{
   struct pipe_draw_info info;

   util_draw_init_info(&info);
   info.indexed = TRUE;
   info.mode = mode;
   info.start = start;
   info.count = count;
   info.index_bias = index_bias;

   pipe->draw_vbo(pipe, &info);
}

static INLINE void
util_draw_arrays_instanced(struct pipe_context *pipe,
                           uint mode, uint start, uint count,
                           uint start_instance,
                           uint instance_count)
{
   struct pipe_draw_info info;

   util_draw_init_info(&info);
   info.mode = mode;
   info.start = start;
   info.count = count;
   info.start_instance = start_instance;
   info.instance_count = instance_count;
   info.min_index = start;
   info.max_index = start + count - 1;

   pipe->draw_vbo(pipe, &info);
}

static INLINE void
util_draw_elements_instanced(struct pipe_context *pipe,
                             int index_bias,
                             uint mode, uint start, uint count,
                             uint start_instance,
                             uint instance_count)
{
   struct pipe_draw_info info;

   util_draw_init_info(&info);
   info.indexed = TRUE;
   info.mode = mode;
   info.start = start;
   info.count = count;
   info.index_bias = index_bias;
   info.start_instance = start_instance;
   info.instance_count = instance_count;

   pipe->draw_vbo(pipe, &info);
}

static INLINE void
util_draw_range_elements(struct pipe_context *pipe,
                         int index_bias,
                         uint min_index,
                         uint max_index,
                         uint mode, uint start, uint count)
{
   struct pipe_draw_info info;

   util_draw_init_info(&info);
   info.indexed = TRUE;
   info.mode = mode;
   info.start = start;
   info.count = count;
   info.index_bias = index_bias;
   info.min_index = min_index;
   info.max_index = max_index;

   pipe->draw_vbo(pipe, &info);
}


unsigned
util_draw_max_index(
      const struct pipe_vertex_buffer *vertex_buffers,
      unsigned nr_vertex_buffers,
      const struct pipe_vertex_element *vertex_elements,
      unsigned nr_vertex_elements,
      const struct pipe_draw_info *info);


#ifdef __cplusplus
}
#endif

#endif /* !U_DRAW_H */
