/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2008 VMware, Inc.  All rights reserved.
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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_pstipple.h"
#include "util/u_inlines.h"
#include "tgsi/tgsi_exec.h"
#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"
#include "sp_clear.h"
#include "sp_context.h"
#include "sp_flush.h"
#include "sp_prim_vbuf.h"
#include "sp_state.h"
#include "sp_surface.h"
#include "sp_tile_cache.h"
#include "sp_tex_tile_cache.h"
#include "sp_texture.h"
#include "sp_query.h"
#include "sp_screen.h"


/**
 * Map any drawing surfaces which aren't already mapped
 */
void
softpipe_map_transfers(struct softpipe_context *sp)
{
   unsigned i;

   for (i = 0; i < sp->framebuffer.nr_cbufs; i++) {
      sp_tile_cache_map_transfers(sp->cbuf_cache[i]);
   }

   sp_tile_cache_map_transfers(sp->zsbuf_cache);
}


/**
 * Unmap any mapped drawing surfaces
 */
void
softpipe_unmap_transfers(struct softpipe_context *sp)
{
   uint i;

   for (i = 0; i < sp->framebuffer.nr_cbufs; i++) {
      sp_tile_cache_unmap_transfers(sp->cbuf_cache[i]);
   }

   sp_tile_cache_unmap_transfers(sp->zsbuf_cache);
}


static void
softpipe_destroy( struct pipe_context *pipe )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   uint i;

#if DO_PSTIPPLE_IN_HELPER_MODULE
   if (softpipe->pstipple.sampler)
      pipe->delete_sampler_state(pipe, softpipe->pstipple.sampler);

   pipe_resource_reference(&softpipe->pstipple.texture, NULL);
   pipe_sampler_view_reference(&softpipe->pstipple.sampler_view, NULL);
#endif

   if (softpipe->draw)
      draw_destroy( softpipe->draw );

   if (softpipe->quad.shade)
      softpipe->quad.shade->destroy( softpipe->quad.shade );

   if (softpipe->quad.depth_test)
      softpipe->quad.depth_test->destroy( softpipe->quad.depth_test );

   if (softpipe->quad.blend)
      softpipe->quad.blend->destroy( softpipe->quad.blend );

   if (softpipe->quad.pstipple)
      softpipe->quad.pstipple->destroy( softpipe->quad.pstipple );

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      sp_destroy_tile_cache(softpipe->cbuf_cache[i]);
      pipe_surface_reference(&softpipe->framebuffer.cbufs[i], NULL);
   }

   sp_destroy_tile_cache(softpipe->zsbuf_cache);
   pipe_surface_reference(&softpipe->framebuffer.zsbuf, NULL);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      sp_destroy_tex_tile_cache(softpipe->fragment_tex_cache[i]);
      pipe_sampler_view_reference(&softpipe->fragment_sampler_views[i], NULL);
   }

   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      sp_destroy_tex_tile_cache(softpipe->vertex_tex_cache[i]);
      pipe_sampler_view_reference(&softpipe->vertex_sampler_views[i], NULL);
   }

   for (i = 0; i < PIPE_MAX_GEOMETRY_SAMPLERS; i++) {
      sp_destroy_tex_tile_cache(softpipe->geometry_tex_cache[i]);
      pipe_sampler_view_reference(&softpipe->geometry_sampler_views[i], NULL);
   }

   for (i = 0; i < PIPE_SHADER_TYPES; i++) {
      uint j;

      for (j = 0; j < PIPE_MAX_CONSTANT_BUFFERS; j++) {
         if (softpipe->constants[i][j]) {
            pipe_resource_reference(&softpipe->constants[i][j], NULL);
         }
      }
   }

   for (i = 0; i < softpipe->num_vertex_buffers; i++) {
      pipe_resource_reference(&softpipe->vertex_buffer[i].buffer, NULL);
   }

   tgsi_exec_machine_destroy(softpipe->fs_machine);

   FREE( softpipe );
}


/**
 * if (the texture is being used as a framebuffer surface)
 *    return SP_REFERENCED_FOR_WRITE
 * else if (the texture is a bound texture source)
 *    return SP_REFERENCED_FOR_READ
 * else
 *    return SP_UNREFERENCED
 */
unsigned int
softpipe_is_resource_referenced( struct pipe_context *pipe,
                                 struct pipe_resource *texture,
                                 unsigned level, int layer)
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   unsigned i;

   if (texture->target == PIPE_BUFFER)
      return SP_UNREFERENCED;

   /* check if any of the bound drawing surfaces are this texture */
   if (softpipe->dirty_render_cache) {
      for (i = 0; i < softpipe->framebuffer.nr_cbufs; i++) {
         if (softpipe->framebuffer.cbufs[i] && 
             softpipe->framebuffer.cbufs[i]->texture == texture) {
            return SP_REFERENCED_FOR_WRITE;
         }
      }
      if (softpipe->framebuffer.zsbuf && 
          softpipe->framebuffer.zsbuf->texture == texture) {
         return SP_REFERENCED_FOR_WRITE;
      }
   }
   
   /* check if any of the tex_cache textures are this texture */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      if (softpipe->fragment_tex_cache[i] &&
          softpipe->fragment_tex_cache[i]->texture == texture)
         return SP_REFERENCED_FOR_READ;
   }
   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      if (softpipe->vertex_tex_cache[i] &&
          softpipe->vertex_tex_cache[i]->texture == texture)
         return SP_REFERENCED_FOR_READ;
   }
   for (i = 0; i < PIPE_MAX_GEOMETRY_SAMPLERS; i++) {
      if (softpipe->geometry_tex_cache[i] &&
          softpipe->geometry_tex_cache[i]->texture == texture)
         return SP_REFERENCED_FOR_READ;
   }

   return SP_UNREFERENCED;
}




static void
softpipe_render_condition( struct pipe_context *pipe,
                           struct pipe_query *query,
                           uint mode )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );

   softpipe->render_cond_query = query;
   softpipe->render_cond_mode = mode;
}



struct pipe_context *
softpipe_create_context( struct pipe_screen *screen,
			 void *priv )
{
   struct softpipe_screen *sp_screen = softpipe_screen(screen);
   struct softpipe_context *softpipe = CALLOC_STRUCT(softpipe_context);
   uint i;

   util_init_math();

   softpipe->dump_fs = debug_get_bool_option( "SOFTPIPE_DUMP_FS", FALSE );
   softpipe->dump_gs = debug_get_bool_option( "SOFTPIPE_DUMP_GS", FALSE );

   softpipe->pipe.winsys = NULL;
   softpipe->pipe.screen = screen;
   softpipe->pipe.destroy = softpipe_destroy;
   softpipe->pipe.priv = priv;

   /* state setters */
   softpipe_init_blend_funcs(&softpipe->pipe);
   softpipe_init_clip_funcs(&softpipe->pipe);
   softpipe_init_query_funcs( softpipe );
   softpipe_init_rasterizer_funcs(&softpipe->pipe);
   softpipe_init_sampler_funcs(&softpipe->pipe);
   softpipe_init_shader_funcs(&softpipe->pipe);
   softpipe_init_streamout_funcs(&softpipe->pipe);
   softpipe_init_texture_funcs( &softpipe->pipe );
   softpipe_init_vertex_funcs(&softpipe->pipe);

   softpipe->pipe.set_framebuffer_state = softpipe_set_framebuffer_state;

   softpipe->pipe.draw_vbo = softpipe_draw_vbo;

   softpipe->pipe.clear = softpipe_clear;
   softpipe->pipe.flush = softpipe_flush_wrapped;

   softpipe->pipe.render_condition = softpipe_render_condition;
   
   softpipe->pipe.create_video_decoder = vl_create_decoder;
   softpipe->pipe.create_video_buffer = vl_video_buffer_create;

   /*
    * Alloc caches for accessing drawing surfaces and textures.
    * Must be before quad stage setup!
    */
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++)
      softpipe->cbuf_cache[i] = sp_create_tile_cache( &softpipe->pipe );
   softpipe->zsbuf_cache = sp_create_tile_cache( &softpipe->pipe );

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      softpipe->fragment_tex_cache[i] = sp_create_tex_tile_cache( &softpipe->pipe );
      if (!softpipe->fragment_tex_cache[i])
         goto fail;
   }

   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      softpipe->vertex_tex_cache[i] = sp_create_tex_tile_cache( &softpipe->pipe );
      if (!softpipe->vertex_tex_cache[i])
         goto fail;
   }

   for (i = 0; i < PIPE_MAX_GEOMETRY_SAMPLERS; i++) {
      softpipe->geometry_tex_cache[i] = sp_create_tex_tile_cache( &softpipe->pipe );
      if (!softpipe->geometry_tex_cache[i])
         goto fail;
   }

   softpipe->fs_machine = tgsi_exec_machine_create();

   /* setup quad rendering stages */
   softpipe->quad.shade = sp_quad_shade_stage(softpipe);
   softpipe->quad.depth_test = sp_quad_depth_test_stage(softpipe);
   softpipe->quad.blend = sp_quad_blend_stage(softpipe);
   softpipe->quad.pstipple = sp_quad_polygon_stipple_stage(softpipe);


   /*
    * Create drawing context and plug our rendering stage into it.
    */
   if (sp_screen->use_llvm)
      softpipe->draw = draw_create(&softpipe->pipe);
   else
      softpipe->draw = draw_create_no_llvm(&softpipe->pipe);
   if (!softpipe->draw) 
      goto fail;

   draw_texture_samplers(softpipe->draw,
                         PIPE_SHADER_VERTEX,
                         PIPE_MAX_VERTEX_SAMPLERS,
                         (struct tgsi_sampler **)
                            softpipe->tgsi.vert_samplers_list);

   draw_texture_samplers(softpipe->draw,
                         PIPE_SHADER_GEOMETRY,
                         PIPE_MAX_GEOMETRY_SAMPLERS,
                         (struct tgsi_sampler **)
                            softpipe->tgsi.geom_samplers_list);

   if (debug_get_bool_option( "SOFTPIPE_NO_RAST", FALSE ))
      softpipe->no_rast = TRUE;

   softpipe->vbuf_backend = sp_create_vbuf_backend(softpipe);
   if (!softpipe->vbuf_backend)
      goto fail;

   softpipe->vbuf = draw_vbuf_stage(softpipe->draw, softpipe->vbuf_backend);
   if (!softpipe->vbuf)
      goto fail;

   draw_set_rasterize_stage(softpipe->draw, softpipe->vbuf);
   draw_set_render(softpipe->draw, softpipe->vbuf_backend);


   /* plug in AA line/point stages */
   draw_install_aaline_stage(softpipe->draw, &softpipe->pipe);
   draw_install_aapoint_stage(softpipe->draw, &softpipe->pipe);

   /* Do polygon stipple w/ texture map + frag prog? */
#if DO_PSTIPPLE_IN_DRAW_MODULE
   draw_install_pstipple_stage(softpipe->draw, &softpipe->pipe);
#endif

   draw_wide_point_sprites(softpipe->draw, TRUE);

   sp_init_surface_functions(softpipe);

#if DO_PSTIPPLE_IN_HELPER_MODULE
   /* create the polgon stipple sampler */
   softpipe->pstipple.sampler = util_pstipple_create_sampler(&softpipe->pipe);
#endif

   return &softpipe->pipe;

 fail:
   softpipe_destroy(&softpipe->pipe);
   return NULL;
}
