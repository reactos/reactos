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
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "pipe/p_context.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_cpu_detect.h"
#include "util/u_inlines.h"
#include "draw_context.h"
#include "draw_vs.h"
#include "draw_gs.h"

#if HAVE_LLVM
#include "gallivm/lp_bld_init.h"
#include "draw_llvm.h"

static boolean
draw_get_option_use_llvm(void)
{
   static boolean first = TRUE;
   static boolean value;
   if (first) {
      first = FALSE;
      value = debug_get_bool_option("DRAW_USE_LLVM", TRUE);

#ifdef PIPE_ARCH_X86
      util_cpu_detect();
      /* require SSE2 due to LLVM PR6960. */
      if (!util_cpu_caps.has_sse2)
         value = FALSE;
#endif
   }
   return value;
}
#endif


/**
 * Create new draw module context with gallivm state for LLVM JIT.
 */
static struct draw_context *
draw_create_context(struct pipe_context *pipe, boolean try_llvm,
                    struct gallivm_state *gallivm)
{
   struct draw_context *draw = CALLOC_STRUCT( draw_context );
   if (draw == NULL)
      goto err_out;

#if HAVE_LLVM
   if (try_llvm && draw_get_option_use_llvm()) {
      if (!gallivm) {
         gallivm = gallivm_create();
         draw->own_gallivm = gallivm;
      }

      if (!gallivm)
         goto err_destroy;

      draw->llvm = draw_llvm_create(draw, gallivm);

      if (!draw->llvm)
         goto err_destroy;
   }
#endif

   if (!draw_init(draw))
      goto err_destroy;

   draw->pipe = pipe;

   return draw;

err_destroy:
   draw_destroy( draw );
err_out:
   return NULL;
}


/**
 * Create new draw module context, with LLVM JIT.
 */
struct draw_context *
draw_create(struct pipe_context *pipe)
{
   return draw_create_context(pipe, TRUE, NULL);
}


/**
 * Create a new draw context, without LLVM JIT.
 */
struct draw_context *
draw_create_no_llvm(struct pipe_context *pipe)
{
   return draw_create_context(pipe, FALSE, NULL);
}


/**
 * Create new draw module context with gallivm state for LLVM JIT.
 */
struct draw_context *
draw_create_gallivm(struct pipe_context *pipe, struct gallivm_state *gallivm)
{
   return draw_create_context(pipe, TRUE, gallivm);
}


boolean draw_init(struct draw_context *draw)
{
   /*
    * Note that several functions compute the clipmask of the predefined
    * formats with hardcoded formulas instead of using these. So modifications
    * here must be reflected there too.
    */

   ASSIGN_4V( draw->plane[0], -1,  0,  0, 1 );
   ASSIGN_4V( draw->plane[1],  1,  0,  0, 1 );
   ASSIGN_4V( draw->plane[2],  0, -1,  0, 1 );
   ASSIGN_4V( draw->plane[3],  0,  1,  0, 1 );
   ASSIGN_4V( draw->plane[4],  0,  0,  1, 1 ); /* yes these are correct */
   ASSIGN_4V( draw->plane[5],  0,  0, -1, 1 ); /* mesa's a bit wonky */
   draw->clip_xy = TRUE;
   draw->clip_z = TRUE;

   draw->pt.user.planes = (float (*) [DRAW_TOTAL_CLIP_PLANES][4]) &(draw->plane[0]);
   draw->reduced_prim = ~0; /* != any of PIPE_PRIM_x */


   if (!draw_pipeline_init( draw ))
      return FALSE;

   if (!draw_pt_init( draw ))
      return FALSE;

   if (!draw_vs_init( draw ))
      return FALSE;

   if (!draw_gs_init( draw ))
      return FALSE;

   return TRUE;
}


void draw_destroy( struct draw_context *draw )
{
   struct pipe_context *pipe;
   int i, j;

   if (!draw)
      return;

   pipe = draw->pipe;

   /* free any rasterizer CSOs that we may have created.
    */
   for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++) {
         if (draw->rasterizer_no_cull[i][j]) {
            pipe->delete_rasterizer_state(pipe, draw->rasterizer_no_cull[i][j]);
         }
      }
   }

   for (i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      pipe_resource_reference(&draw->pt.vertex_buffer[i].buffer, NULL);
   }

   /* Not so fast -- we're just borrowing this at the moment.
    * 
   if (draw->render)
      draw->render->destroy( draw->render );
   */

   draw_pipeline_destroy( draw );
   draw_pt_destroy( draw );
   draw_vs_destroy( draw );
   draw_gs_destroy( draw );
#ifdef HAVE_LLVM
   if (draw->llvm)
      draw_llvm_destroy( draw->llvm );

   if (draw->own_gallivm)
      gallivm_destroy(draw->own_gallivm);
#endif

   FREE( draw );
}



void draw_flush( struct draw_context *draw )
{
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );
}


/**
 * Specify the Minimum Resolvable Depth factor for polygon offset.
 * This factor potentially depends on the number of Z buffer bits,
 * the rasterization algorithm and the arithmetic performed on Z
 * values between vertex shading and rasterization.  It will vary
 * from one driver to another.
 */
void draw_set_mrd(struct draw_context *draw, double mrd)
{
   draw->mrd = mrd;
}


static void update_clip_flags( struct draw_context *draw )
{
   draw->clip_xy = !draw->driver.bypass_clip_xy;
   draw->guard_band_xy = (!draw->driver.bypass_clip_xy &&
                          draw->driver.guard_band_xy);
   draw->clip_z = (!draw->driver.bypass_clip_z &&
                   draw->rasterizer && draw->rasterizer->depth_clip);
   draw->clip_user = draw->rasterizer &&
                     draw->rasterizer->clip_plane_enable != 0;
}

/**
 * Register new primitive rasterization/rendering state.
 * This causes the drawing pipeline to be rebuilt.
 */
void draw_set_rasterizer_state( struct draw_context *draw,
                                const struct pipe_rasterizer_state *raster,
                                void *rast_handle )
{
   if (!draw->suspend_flushing) {
      draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

      draw->rasterizer = raster;
      draw->rast_handle = rast_handle;
      update_clip_flags(draw);
   }
}

/* With a little more work, llvmpipe will be able to turn this off and
 * do its own x/y clipping.  
 *
 * Some hardware can turn off clipping altogether - in particular any
 * hardware with a TNL unit can do its own clipping, even if it is
 * relying on the draw module for some other reason.
 */
void draw_set_driver_clipping( struct draw_context *draw,
                               boolean bypass_clip_xy,
                               boolean bypass_clip_z,
                               boolean guard_band_xy)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   draw->driver.bypass_clip_xy = bypass_clip_xy;
   draw->driver.bypass_clip_z = bypass_clip_z;
   draw->driver.guard_band_xy = guard_band_xy;
   update_clip_flags(draw);
}


/** 
 * Plug in the primitive rendering/rasterization stage (which is the last
 * stage in the drawing pipeline).
 * This is provided by the device driver.
 */
void draw_set_rasterize_stage( struct draw_context *draw,
                               struct draw_stage *stage )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   draw->pipeline.rasterize = stage;
}


/**
 * Set the draw module's clipping state.
 */
void draw_set_clip_state( struct draw_context *draw,
                          const struct pipe_clip_state *clip )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   memcpy(&draw->plane[6], clip->ucp, sizeof(clip->ucp));
}


/**
 * Set the draw module's viewport state.
 */
void draw_set_viewport_state( struct draw_context *draw,
                              const struct pipe_viewport_state *viewport )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->viewport = *viewport; /* struct copy */
   draw->identity_viewport = (viewport->scale[0] == 1.0f &&
                              viewport->scale[1] == 1.0f &&
                              viewport->scale[2] == 1.0f &&
                              viewport->scale[3] == 1.0f &&
                              viewport->translate[0] == 0.0f &&
                              viewport->translate[1] == 0.0f &&
                              viewport->translate[2] == 0.0f &&
                              viewport->translate[3] == 0.0f);

   draw_vs_set_viewport( draw, viewport );
}



void
draw_set_vertex_buffers(struct draw_context *draw,
                        unsigned count,
                        const struct pipe_vertex_buffer *buffers)
{
   assert(count <= PIPE_MAX_ATTRIBS);

   util_copy_vertex_buffers(draw->pt.vertex_buffer,
                            &draw->pt.nr_vertex_buffers,
                            buffers, count);
}


void
draw_set_vertex_elements(struct draw_context *draw,
                         unsigned count,
                         const struct pipe_vertex_element *elements)
{
   assert(count <= PIPE_MAX_ATTRIBS);

   memcpy(draw->pt.vertex_element, elements, count * sizeof(elements[0]));
   draw->pt.nr_vertex_elements = count;
}


/**
 * Tell drawing context where to find mapped vertex buffers.
 */
void
draw_set_mapped_vertex_buffer(struct draw_context *draw,
                              unsigned attr, const void *buffer)
{
   draw->pt.user.vbuffer[attr] = buffer;
}


void
draw_set_mapped_constant_buffer(struct draw_context *draw,
                                unsigned shader_type,
                                unsigned slot,
                                const void *buffer,
                                unsigned size )
{
   debug_assert(shader_type == PIPE_SHADER_VERTEX ||
                shader_type == PIPE_SHADER_GEOMETRY);
   debug_assert(slot < PIPE_MAX_CONSTANT_BUFFERS);

   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      draw->pt.user.vs_constants[slot] = buffer;
      draw->pt.user.vs_constants_size[slot] = size;
      draw_vs_set_constants(draw, slot, buffer, size);
      break;
   case PIPE_SHADER_GEOMETRY:
      draw->pt.user.gs_constants[slot] = buffer;
      draw->pt.user.gs_constants_size[slot] = size;
      draw_gs_set_constants(draw, slot, buffer, size);
      break;
   default:
      assert(0 && "invalid shader type in draw_set_mapped_constant_buffer");
   }
}


/**
 * Tells the draw module to draw points with triangles if their size
 * is greater than this threshold.
 */
void
draw_wide_point_threshold(struct draw_context *draw, float threshold)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.wide_point_threshold = threshold;
}


/**
 * Should the draw module handle point->quad conversion for drawing sprites?
 */
void
draw_wide_point_sprites(struct draw_context *draw, boolean draw_sprite)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.wide_point_sprites = draw_sprite;
}


/**
 * Tells the draw module to draw lines with triangles if their width
 * is greater than this threshold.
 */
void
draw_wide_line_threshold(struct draw_context *draw, float threshold)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.wide_line_threshold = roundf(threshold);
}


/**
 * Tells the draw module whether or not to implement line stipple.
 */
void
draw_enable_line_stipple(struct draw_context *draw, boolean enable)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.line_stipple = enable;
}


/**
 * Tells draw module whether to convert points to quads for sprite mode.
 */
void
draw_enable_point_sprites(struct draw_context *draw, boolean enable)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.point_sprite = enable;
}


void
draw_set_force_passthrough( struct draw_context *draw, boolean enable )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->force_passthrough = enable;
}



/**
 * Allocate an extra vertex/geometry shader vertex attribute, if it doesn't
 * exist already.
 *
 * This is used by some of the optional draw module stages such
 * as wide_point which may need to allocate additional generic/texcoord
 * attributes.
 */
int
draw_alloc_extra_vertex_attrib(struct draw_context *draw,
                               uint semantic_name, uint semantic_index)
{
   int slot;
   uint num_outputs;
   uint n;

   slot = draw_find_shader_output(draw, semantic_name, semantic_index);
   if (slot > 0) {
      return slot;
   }

   num_outputs = draw_current_shader_outputs(draw);
   n = draw->extra_shader_outputs.num;

   assert(n < Elements(draw->extra_shader_outputs.semantic_name));

   draw->extra_shader_outputs.semantic_name[n] = semantic_name;
   draw->extra_shader_outputs.semantic_index[n] = semantic_index;
   draw->extra_shader_outputs.slot[n] = num_outputs + n;
   draw->extra_shader_outputs.num++;

   return draw->extra_shader_outputs.slot[n];
}


/**
 * Remove all extra vertex attributes that were allocated with
 * draw_alloc_extra_vertex_attrib().
 */
void
draw_remove_extra_vertex_attribs(struct draw_context *draw)
{
   draw->extra_shader_outputs.num = 0;
}


/**
 * If a geometry shader is present, return its info, else the vertex shader's
 * info.
 */
struct tgsi_shader_info *
draw_get_shader_info(const struct draw_context *draw)
{

   if (draw->gs.geometry_shader) {
      return &draw->gs.geometry_shader->info;
   } else {
      return &draw->vs.vertex_shader->info;
   }
}


/**
 * Ask the draw module for the location/slot of the given vertex attribute in
 * a post-transformed vertex.
 *
 * With this function, drivers that use the draw module should have no reason
 * to track the current vertex/geometry shader.
 *
 * Note that the draw module may sometimes generate vertices with extra
 * attributes (such as texcoords for AA lines).  The driver can call this
 * function to find those attributes.
 *
 * Zero is returned if the attribute is not found since this is
 * a don't care / undefined situtation.  Returning -1 would be a bit more
 * work for the drivers.
 */
int
draw_find_shader_output(const struct draw_context *draw,
                        uint semantic_name, uint semantic_index)
{
   const struct tgsi_shader_info *info = draw_get_shader_info(draw);
   uint i;

   for (i = 0; i < info->num_outputs; i++) {
      if (info->output_semantic_name[i] == semantic_name &&
          info->output_semantic_index[i] == semantic_index)
         return i;
   }

   /* Search the extra vertex attributes */
   for (i = 0; i < draw->extra_shader_outputs.num; i++) {
      if (draw->extra_shader_outputs.semantic_name[i] == semantic_name &&
          draw->extra_shader_outputs.semantic_index[i] == semantic_index) {
         return draw->extra_shader_outputs.slot[i];
      }
   }

   return 0;
}


/**
 * Return total number of the shader outputs.  This function is similar to
 * draw_current_shader_outputs() but this function also counts any extra
 * vertex/geometry output attributes that may be filled in by some draw
 * stages (such as AA point, AA line).
 *
 * If geometry shader is present, its output will be returned,
 * if not vertex shader is used.
 */
uint
draw_num_shader_outputs(const struct draw_context *draw)
{
   const struct tgsi_shader_info *info = draw_get_shader_info(draw);
   uint count;

   count = info->num_outputs;
   count += draw->extra_shader_outputs.num;

   return count;
}


/**
 * Provide TGSI sampler objects for vertex/geometry shaders that use
 * texture fetches.
 * This might only be used by software drivers for the time being.
 */
void
draw_texture_samplers(struct draw_context *draw,
                      uint shader,
                      uint num_samplers,
                      struct tgsi_sampler **samplers)
{
   if (shader == PIPE_SHADER_VERTEX) {
      draw->vs.num_samplers = num_samplers;
      draw->vs.samplers = samplers;
   } else {
      debug_assert(shader == PIPE_SHADER_GEOMETRY);
      draw->gs.num_samplers = num_samplers;
      draw->gs.samplers = samplers;
   }
}




void draw_set_render( struct draw_context *draw, 
		      struct vbuf_render *render )
{
   draw->render = render;
}


void
draw_set_index_buffer(struct draw_context *draw,
                      const struct pipe_index_buffer *ib)
{
   if (ib)
      memcpy(&draw->pt.index_buffer, ib, sizeof(draw->pt.index_buffer));
   else
      memset(&draw->pt.index_buffer, 0, sizeof(draw->pt.index_buffer));
}


/**
 * Tell drawing context where to find mapped index/element buffer.
 */
void
draw_set_mapped_index_buffer(struct draw_context *draw,
                             const void *elements)
{
    draw->pt.user.elts = elements;
}


/* Revamp me please:
 */
void draw_do_flush( struct draw_context *draw, unsigned flags )
{
   if (!draw->suspend_flushing)
   {
      assert(!draw->flushing); /* catch inadvertant recursion */

      draw->flushing = TRUE;

      draw_pipeline_flush( draw, flags );

      draw->reduced_prim = ~0; /* is reduced_prim needed any more? */
      
      draw->flushing = FALSE;
   }
}


/**
 * Return the number of output attributes produced by the geometry
 * shader, if present.  If no geometry shader, return the number of
 * outputs from the vertex shader.
 * \sa draw_num_shader_outputs
 */
uint
draw_current_shader_outputs(const struct draw_context *draw)
{
   if (draw->gs.geometry_shader)
      return draw->gs.num_gs_outputs;
   return draw->vs.num_vs_outputs;
}


/**
 * Return the index of the shader output which will contain the
 * vertex position.
 */
uint
draw_current_shader_position_output(const struct draw_context *draw)
{
   if (draw->gs.geometry_shader)
      return draw->gs.position_output;
   return draw->vs.position_output;
}


/**
 * Return the index of the shader output which will contain the
 * vertex position.
 */
uint
draw_current_shader_clipvertex_output(const struct draw_context *draw)
{
   return draw->vs.clipvertex_output;
}

uint
draw_current_shader_clipdistance_output(const struct draw_context *draw, int index)
{
   return draw->vs.clipdistance_output[index];
}

/**
 * Return a pointer/handle for a driver/CSO rasterizer object which
 * disabled culling, stippling, unfilled tris, etc.
 * This is used by some pipeline stages (such as wide_point, aa_line
 * and aa_point) which convert points/lines into triangles.  In those
 * cases we don't want to accidentally cull the triangles.
 *
 * \param scissor  should the rasterizer state enable scissoring?
 * \param flatshade  should the rasterizer state use flat shading?
 * \return  rasterizer CSO handle
 */
void *
draw_get_rasterizer_no_cull( struct draw_context *draw,
                             boolean scissor,
                             boolean flatshade )
{
   if (!draw->rasterizer_no_cull[scissor][flatshade]) {
      /* create now */
      struct pipe_context *pipe = draw->pipe;
      struct pipe_rasterizer_state rast;

      memset(&rast, 0, sizeof(rast));
      rast.scissor = scissor;
      rast.flatshade = flatshade;
      rast.front_ccw = 1;
      rast.gl_rasterization_rules = draw->rasterizer->gl_rasterization_rules;

      draw->rasterizer_no_cull[scissor][flatshade] =
         pipe->create_rasterizer_state(pipe, &rast);
   }
   return draw->rasterizer_no_cull[scissor][flatshade];
}

void
draw_set_mapped_so_targets(struct draw_context *draw,
                           int num_targets,
                           struct draw_so_target *targets[PIPE_MAX_SO_BUFFERS])
{
   int i;

   for (i = 0; i < num_targets; i++)
      draw->so.targets[i] = targets[i];
   for (i = num_targets; i < PIPE_MAX_SO_BUFFERS; i++)
      draw->so.targets[i] = NULL;

   draw->so.num_targets = num_targets;
}

void
draw_set_mapped_so_buffers(struct draw_context *draw,
                           void *buffers[PIPE_MAX_SO_BUFFERS],
                           unsigned num_buffers)
{
}

void
draw_set_so_state(struct draw_context *draw,
                  struct pipe_stream_output_info *state)
{
   memcpy(&draw->so.state,
          state,
          sizeof(struct pipe_stream_output_info));
}

void
draw_set_sampler_views(struct draw_context *draw,
                       struct pipe_sampler_view **views,
                       unsigned num)
{
   unsigned i;

   debug_assert(num <= PIPE_MAX_VERTEX_SAMPLERS);

   for (i = 0; i < num; ++i)
      draw->sampler_views[i] = views[i];
   for (i = num; i < PIPE_MAX_VERTEX_SAMPLERS; ++i)
      draw->sampler_views[i] = NULL;

   draw->num_sampler_views = num;
}

void
draw_set_samplers(struct draw_context *draw,
                  struct pipe_sampler_state **samplers,
                  unsigned num)
{
   unsigned i;

   debug_assert(num <= PIPE_MAX_VERTEX_SAMPLERS);

   for (i = 0; i < num; ++i)
      draw->samplers[i] = samplers[i];
   for (i = num; i < PIPE_MAX_VERTEX_SAMPLERS; ++i)
      draw->samplers[i] = NULL;

   draw->num_samplers = num;

#ifdef HAVE_LLVM
   if (draw->llvm)
      draw_llvm_set_sampler_state(draw);
#endif
}

void
draw_set_mapped_texture(struct draw_context *draw,
                        unsigned sampler_idx,
                        uint32_t width, uint32_t height, uint32_t depth,
                        uint32_t first_level, uint32_t last_level,
                        uint32_t row_stride[PIPE_MAX_TEXTURE_LEVELS],
                        uint32_t img_stride[PIPE_MAX_TEXTURE_LEVELS],
                        const void *data[PIPE_MAX_TEXTURE_LEVELS])
{
#ifdef HAVE_LLVM
   if(draw->llvm)
      draw_llvm_set_mapped_texture(draw,
                                sampler_idx,
                                width, height, depth, first_level, last_level,
                                row_stride, img_stride, data);
#endif
}
