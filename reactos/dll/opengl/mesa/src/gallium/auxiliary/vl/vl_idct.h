/**************************************************************************
 *
 * Copyright 2010 Christian König
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

#ifndef vl_idct_h
#define vl_idct_h

#include "pipe/p_state.h"

#include "tgsi/tgsi_ureg.h"

/* shader based inverse distinct cosinus transformation
 * expect usage of vl_vertex_buffers as a todo list
 */
struct vl_idct
{
   struct pipe_context *pipe;

   unsigned buffer_width;
   unsigned buffer_height;
   unsigned nr_of_render_targets;

   void *rs_state;
   void *blend;

   void *samplers[2];

   void *vs_mismatch, *fs_mismatch;
   void *vs, *fs;

   struct pipe_sampler_view *matrix;
   struct pipe_sampler_view *transpose;
};

/* a set of buffers to work with */
struct vl_idct_buffer
{
   struct pipe_viewport_state viewport_mismatch;
   struct pipe_viewport_state viewport;

   struct pipe_framebuffer_state fb_state_mismatch;
   struct pipe_framebuffer_state fb_state;

   union
   {
      struct pipe_sampler_view *all[4];
      struct pipe_sampler_view *stage[2][2];
      struct {
         struct pipe_sampler_view *source, *matrix;
         struct pipe_sampler_view *intermediate, *transpose;
      } individual;
   } sampler_views;
};

/* upload the idct matrix, which can be shared by all idct instances of a pipe */
struct pipe_sampler_view *
vl_idct_upload_matrix(struct pipe_context *pipe, float scale);

void
vl_idct_stage2_vert_shader(struct vl_idct *idct, struct ureg_program *shader,
                           unsigned first_output, struct ureg_dst tex);

void
vl_idct_stage2_frag_shader(struct vl_idct *idct, struct ureg_program *shader,
                           unsigned first_input, struct ureg_dst fragment);

/* init an idct instance */
bool
vl_idct_init(struct vl_idct *idct, struct pipe_context *pipe,
             unsigned buffer_width, unsigned buffer_height,
             unsigned nr_of_render_targets,
             struct pipe_sampler_view *matrix,
             struct pipe_sampler_view *transpose);

/* destroy an idct instance */
void
vl_idct_cleanup(struct vl_idct *idct);

/* init a buffer assosiated with agiven idct instance */
bool
vl_idct_init_buffer(struct vl_idct *idct, struct vl_idct_buffer *buffer,
                    struct pipe_sampler_view *source,
                    struct pipe_sampler_view *intermediate);

/* cleanup a buffer of an idct instance */
void
vl_idct_cleanup_buffer(struct vl_idct_buffer *buffer);

/* flush the buffer and start rendering, vertex buffers needs to be setup before calling this */
void
vl_idct_flush(struct vl_idct *idct, struct vl_idct_buffer *buffer, unsigned num_verts);

void
vl_idct_prepare_stage2(struct vl_idct *idct, struct vl_idct_buffer *buffer);

#endif
