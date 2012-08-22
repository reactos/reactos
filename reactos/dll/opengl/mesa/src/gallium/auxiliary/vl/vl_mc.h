/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#ifndef vl_mc_h
#define vl_mc_h

#include "pipe/p_state.h"
#include "pipe/p_video_state.h"

#include "tgsi/tgsi_ureg.h"

#include "vl_defines.h"
#include "vl_types.h"

#define VL_MC_NUM_BLENDERS (1 << VL_MAX_PLANES)

struct pipe_context;

struct vl_mc
{
   struct pipe_context *pipe;
   unsigned buffer_width;
   unsigned buffer_height;
   unsigned macroblock_size;

   void *rs_state;

   void *blend_clear[VL_MC_NUM_BLENDERS];
   void *blend_add[VL_MC_NUM_BLENDERS];
   void *blend_sub[VL_MC_NUM_BLENDERS];
   void *vs_ref, *vs_ycbcr;
   void *fs_ref, *fs_ycbcr, *fs_ycbcr_sub;
   void *sampler_ref;
};

struct vl_mc_buffer
{
   bool surface_cleared;

   struct pipe_viewport_state viewport;
   struct pipe_framebuffer_state fb_state;
};

typedef void (*vl_mc_ycbcr_vert_shader)(void *priv, struct vl_mc *mc,
                                        struct ureg_program *shader,
                                        unsigned first_output,
                                        struct ureg_dst tex);

typedef void (*vl_mc_ycbcr_frag_shader)(void *priv, struct vl_mc *mc,
                                        struct ureg_program *shader,
                                        unsigned first_input,
                                        struct ureg_dst dst);

bool vl_mc_init(struct vl_mc *renderer, struct pipe_context *pipe,
                unsigned picture_width, unsigned picture_height,
                unsigned macroblock_size, float scale,
                vl_mc_ycbcr_vert_shader vs_callback,
                vl_mc_ycbcr_frag_shader fs_callback,
                void *callback_priv);

void vl_mc_cleanup(struct vl_mc *renderer);

bool vl_mc_init_buffer(struct vl_mc *renderer, struct vl_mc_buffer *buffer);

void vl_mc_cleanup_buffer(struct vl_mc_buffer *buffer);

void vl_mc_set_surface(struct vl_mc_buffer *buffer, struct pipe_surface *surface);

void vl_mc_render_ref(struct vl_mc *renderer, struct vl_mc_buffer *buffer, struct pipe_sampler_view *ref);

void vl_mc_render_ycbcr(struct vl_mc *renderer, struct vl_mc_buffer *buffer, unsigned component, unsigned num_instances);

#endif /* vl_mc_h */
