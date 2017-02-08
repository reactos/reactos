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

#ifndef SP_CONTEXT_H
#define SP_CONTEXT_H

#include "pipe/p_context.h"

#include "draw/draw_vertex.h"

#include "sp_quad_pipe.h"


/** Do polygon stipple in the draw module? */
#define DO_PSTIPPLE_IN_DRAW_MODULE 0

/** Do polygon stipple with the util module? */
#define DO_PSTIPPLE_IN_HELPER_MODULE 1


struct softpipe_vbuf_render;
struct draw_context;
struct draw_stage;
struct softpipe_tile_cache;
struct softpipe_tex_tile_cache;
struct sp_fragment_shader;
struct sp_vertex_shader;
struct sp_velems_state;
struct sp_so_state;

struct softpipe_context {
   struct pipe_context pipe;  /**< base class */

   /** Constant state objects */
   struct pipe_blend_state *blend;
   struct pipe_sampler_state *fragment_samplers[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_state *vertex_samplers[PIPE_MAX_VERTEX_SAMPLERS];
   struct pipe_sampler_state *geometry_samplers[PIPE_MAX_GEOMETRY_SAMPLERS];
   struct pipe_depth_stencil_alpha_state *depth_stencil;
   struct pipe_rasterizer_state *rasterizer;
   struct sp_fragment_shader *fs;
   struct sp_fragment_shader_variant *fs_variant;
   struct sp_vertex_shader *vs;
   struct sp_geometry_shader *gs;
   struct sp_velems_state *velems;
   struct sp_so_state *so;

   /** Other rendering state */
   struct pipe_blend_color blend_color;
   struct pipe_blend_color blend_color_clamped;
   struct pipe_stencil_ref stencil_ref;
   struct pipe_clip_state clip;
   struct pipe_resource *constants[PIPE_SHADER_TYPES][PIPE_MAX_CONSTANT_BUFFERS];
   struct pipe_framebuffer_state framebuffer;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct pipe_sampler_view *fragment_sampler_views[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_view *vertex_sampler_views[PIPE_MAX_VERTEX_SAMPLERS];
   struct pipe_sampler_view *geometry_sampler_views[PIPE_MAX_GEOMETRY_SAMPLERS];
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
   struct pipe_index_buffer index_buffer;

   struct draw_so_target *so_targets[PIPE_MAX_SO_BUFFERS];
   int num_so_targets;
   
   struct pipe_query_data_so_statistics so_stats;
   unsigned num_primitives_generated;

   unsigned num_fragment_samplers;
   unsigned num_fragment_sampler_views;
   unsigned num_vertex_samplers;
   unsigned num_vertex_sampler_views;
   unsigned num_geometry_samplers;
   unsigned num_geometry_sampler_views;
   unsigned num_vertex_buffers;

   unsigned dirty; /**< Mask of SP_NEW_x flags */

   /* Counter for occlusion queries.  Note this supports overlapping
    * queries.
    */
   uint64_t occlusion_count;
   unsigned active_query_count;

   /** Mapped vertex buffers */
   ubyte *mapped_vbuffer[PIPE_MAX_ATTRIBS];

   /** Mapped constant buffers */
   const void *mapped_constants[PIPE_SHADER_TYPES][PIPE_MAX_CONSTANT_BUFFERS];
   unsigned const_buffer_size[PIPE_SHADER_TYPES][PIPE_MAX_CONSTANT_BUFFERS];

   /** Vertex format */
   struct vertex_info vertex_info;
   struct vertex_info vertex_info_vbuf;

   /** Which vertex shader output slot contains point size */
   int psize_slot;

   /** The reduced version of the primitive supplied by the state tracker */
   unsigned reduced_api_prim;

   /** Derived information about which winding orders to cull */
   unsigned cull_mode;

   /**
    * The reduced primitive after unfilled triangles, wide-line decomposition,
    * etc, are taken into account.  This is the primitive type that's actually
    * rasterized.
    */
   unsigned reduced_prim;

   /** Derived from scissor and surface bounds: */
   struct pipe_scissor_state cliprect;

   unsigned line_stipple_counter;

   /** Conditional query object and mode */
   struct pipe_query *render_cond_query;
   uint render_cond_mode;

   /** Polygon stipple items */
   struct {
      struct pipe_resource *texture;
      struct pipe_sampler_state *sampler;
      struct pipe_sampler_view *sampler_view;
   } pstipple;

   /** Software quad rendering pipeline */
   struct {
      struct quad_stage *shade;
      struct quad_stage *depth_test;
      struct quad_stage *blend;
      struct quad_stage *pstipple;
      struct quad_stage *first; /**< points to one of the above stages */
   } quad;

   /** TGSI exec things */
   struct {
      struct sp_sampler_variant *geom_samplers_list[PIPE_MAX_GEOMETRY_SAMPLERS];
      struct sp_sampler_variant *vert_samplers_list[PIPE_MAX_VERTEX_SAMPLERS];
      struct sp_sampler_variant *frag_samplers_list[PIPE_MAX_SAMPLERS];
   } tgsi;

   struct tgsi_exec_machine *fs_machine;

   /** The primitive drawing context */
   struct draw_context *draw;

   /** Draw module backend */
   struct vbuf_render *vbuf_backend;
   struct draw_stage *vbuf;

   boolean dirty_render_cache;

   struct softpipe_tile_cache *cbuf_cache[PIPE_MAX_COLOR_BUFS];
   struct softpipe_tile_cache *zsbuf_cache;

   unsigned tex_timestamp;
   struct softpipe_tex_tile_cache *fragment_tex_cache[PIPE_MAX_SAMPLERS];
   struct softpipe_tex_tile_cache *vertex_tex_cache[PIPE_MAX_VERTEX_SAMPLERS];
   struct softpipe_tex_tile_cache *geometry_tex_cache[PIPE_MAX_GEOMETRY_SAMPLERS];

   unsigned dump_fs : 1;
   unsigned dump_gs : 1;
   unsigned no_rast : 1;
};


static INLINE struct softpipe_context *
softpipe_context( struct pipe_context *pipe )
{
   return (struct softpipe_context *)pipe;
}

void
softpipe_reset_sampler_variants(struct softpipe_context *softpipe);

struct pipe_context *
softpipe_create_context( struct pipe_screen *, void *priv );


#define SP_UNREFERENCED         0
#define SP_REFERENCED_FOR_READ  (1 << 0)
#define SP_REFERENCED_FOR_WRITE (1 << 1)

unsigned int
softpipe_is_resource_referenced( struct pipe_context *pipe,
                                 struct pipe_resource *texture,
                                 unsigned level, int layer);

#endif /* SP_CONTEXT_H */
