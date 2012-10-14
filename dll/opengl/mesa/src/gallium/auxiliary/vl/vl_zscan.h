/**************************************************************************
 *
 * Copyright 2011 Christian König
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

#ifndef vl_zscan_h
#define vl_zscan_h

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

/*
 * shader based zscan and quantification
 * expect usage of vl_vertex_buffers as a todo list
 */
struct vl_zscan
{
   struct pipe_context *pipe;

   unsigned buffer_width;
   unsigned buffer_height;

   unsigned num_channels;

   unsigned blocks_per_line;
   unsigned blocks_total;

   void *rs_state;
   void *blend;

   void *samplers[3];

   void *vs, *fs;
};

struct vl_zscan_buffer
{
   struct pipe_viewport_state viewport;
   struct pipe_framebuffer_state fb_state;

   struct pipe_sampler_view *src, *layout, *quant;
   struct pipe_surface *dst;
};

extern const int vl_zscan_linear[];
extern const int vl_zscan_normal[];
extern const int vl_zscan_alternate[];

struct pipe_sampler_view *
vl_zscan_layout(struct pipe_context *pipe, const int layout[64], unsigned blocks_per_line);

bool
vl_zscan_init(struct vl_zscan *zscan, struct pipe_context *pipe,
              unsigned buffer_width, unsigned buffer_height,
              unsigned blocks_per_line, unsigned blocks_total,
              unsigned num_channels);

void
vl_zscan_cleanup(struct vl_zscan *zscan);

bool
vl_zscan_init_buffer(struct vl_zscan *zscan, struct vl_zscan_buffer *buffer,
                     struct pipe_sampler_view *src, struct pipe_surface *dst);

void
vl_zscan_cleanup_buffer(struct vl_zscan_buffer *buffer);

void
vl_zscan_set_layout(struct vl_zscan_buffer *buffer, struct pipe_sampler_view *layout);

void
vl_zscan_upload_quant(struct vl_zscan *zscan, struct vl_zscan_buffer *buffer,
                      const uint8_t matrix[64], bool intra);

void
vl_zscan_render(struct vl_zscan *zscan, struct vl_zscan_buffer *buffer, unsigned num_instances);

#endif
