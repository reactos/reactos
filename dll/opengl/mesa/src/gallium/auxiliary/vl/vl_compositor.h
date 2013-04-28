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

#ifndef vl_compositor_h
#define vl_compositor_h

#include "pipe/p_state.h"
#include "pipe/p_video_decoder.h"
#include "pipe/p_video_state.h"

#include "util/u_rect.h"

#include "vl_types.h"

struct pipe_context;

/**
 * composing and displaying of image data
 */

#define VL_COMPOSITOR_MAX_LAYERS 16

struct vl_compositor_layer
{
   bool clearing;

   void *fs;
   void *samplers[3];
   void *blend;

   struct pipe_sampler_view *sampler_views[3];
   struct {
      struct vertex2f tl, br;
   } src, dst;
};

struct vl_compositor
{
   struct pipe_context *pipe;

   struct pipe_framebuffer_state fb_state;
   struct pipe_viewport_state viewport;
   struct pipe_scissor_state scissor;
   struct pipe_vertex_buffer vertex_buf;
   struct pipe_resource *csc_matrix;

   void *sampler_linear;
   void *sampler_nearest;
   void *blend_clear, *blend_add;
   void *rast;
   void *dsa;
   void *vertex_elems_state;

   void *vs;
   void *fs_video_buffer;
   void *fs_rgba;

   struct {
      void *rgb;
      void *yuv;
   } fs_palette;

   union pipe_color_union clear_color;

   unsigned used_layers:VL_COMPOSITOR_MAX_LAYERS;
   struct vl_compositor_layer layers[VL_COMPOSITOR_MAX_LAYERS];
};

/**
 * initialize this compositor
 */
bool
vl_compositor_init(struct vl_compositor *compositor, struct pipe_context *pipe);

/**
 * set yuv -> rgba conversion matrix
 */
void
vl_compositor_set_csc_matrix(struct vl_compositor *compositor, const float mat[16]);

/**
 * reset dirty area, so it's cleared with the clear colour
 */
void
vl_compositor_reset_dirty_area(struct u_rect *dirty);

/**
 * set the clear color
 */
void
vl_compositor_set_clear_color(struct vl_compositor *compositor, union pipe_color_union *color);

/**
 * get the clear color
 */
void
vl_compositor_get_clear_color(struct vl_compositor *compositor, union pipe_color_union *color);

/**
 * set overlay samplers
 */
/*@{*/

/**
 * reset all currently set layers
 */
void
vl_compositor_clear_layers(struct vl_compositor *compositor);

/**
 * set the blender used to render a layer
 */
void
vl_compositor_set_layer_blend(struct vl_compositor *compositor,
                              unsigned layer, void *blend, bool is_clearing);

/**
 * set a video buffer as a layer to render
 */
void
vl_compositor_set_buffer_layer(struct vl_compositor *compositor,
                               unsigned layer,
                               struct pipe_video_buffer *buffer,
                               struct pipe_video_rect *src_rect,
                               struct pipe_video_rect *dst_rect);

/**
 * set a paletted sampler as a layer to render
 */
void
vl_compositor_set_palette_layer(struct vl_compositor *compositor,
                                unsigned layer,
                                struct pipe_sampler_view *indexes,
                                struct pipe_sampler_view *palette,
                                struct pipe_video_rect *src_rect,
                                struct pipe_video_rect *dst_rect,
                                bool include_color_conversion);

/**
 * set a rgba sampler as a layer to render
 */
void
vl_compositor_set_rgba_layer(struct vl_compositor *compositor,
                             unsigned layer,
                             struct pipe_sampler_view *rgba,
                             struct pipe_video_rect *src_rect,
                             struct pipe_video_rect *dst_rect);

/*@}*/

/**
 * render the layers to the frontbuffer
 */
void
vl_compositor_render(struct vl_compositor   *compositor,
                     struct pipe_surface    *dst_surface,
                     struct pipe_video_rect *dst_area,
                     struct pipe_video_rect *dst_clip,
                     struct u_rect          *dirty_area);

/**
 * destroy this compositor
 */
void
vl_compositor_cleanup(struct vl_compositor *compositor);

#endif /* vl_compositor_h */
