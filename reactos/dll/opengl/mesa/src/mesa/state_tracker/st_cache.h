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
  *   Zack Rusin <zack@tungstengraphics.com>
  */

#ifndef ST_CACHE_H
#define ST_CACHE_H

struct pipe_blend_state;
struct pipe_depth_stencil_alpha_state;
struct pipe_rasterizer_state;
struct pipe_sampler_state;
struct pipe_shader_state;
struct st_context;


const struct cso_blend *
st_cached_blend_state(struct st_context *st,
                      const struct pipe_blend_state *blend);

const struct cso_sampler *
st_cached_sampler_state(struct st_context *st,
                        const struct pipe_sampler_state *sampler);

const struct cso_depth_stencil_alpha *
st_cached_depth_stencil_alpha_state(struct st_context *st,
                              const struct pipe_depth_stencil_alpha_state *depth_stencil);

const struct cso_rasterizer *
st_cached_rasterizer_state(struct st_context *st,
                           const struct pipe_rasterizer_state *raster);

const struct cso_fragment_shader *
st_cached_fs_state(struct st_context *st,
                   const struct pipe_shader_state *templ);


const struct cso_vertex_shader *
st_cached_vs_state(struct st_context *st,
                   const struct pipe_shader_state *templ);

#endif
